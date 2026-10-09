"""Exact candidate/retail reference pairing for compiler-private literals.

Adapts HoMM1 Buka d3c9297135800ee0d198b292f2424464d20b6f5c
`_paired_votes`; this stricter path also proves instruction bytes and field
positions before assigning an address. Initialized source globals may also
prove a literal through their reviewed pointer fields and exact remaining
bytes. No raw integer/zero-content scan.
"""
from __future__ import annotations
import struct
from collections import defaultdict
from hobbit.core.msvc_names import mask


def _exact_instruction_alignment_tail(obj, section, payload, start, size, end,
                                      rva, member_starts, image):
    """Hobbit VC6 extension of the donor's unchanged alignment-window proof.

    A three-byte LEA reg,[same-reg+0] or two-byte MOV reg,reg precedes
    some DWORD switch tables. Both are actual unchanged PC instructions.
    Prove the real next boundary, unchanged PC bytes and absence of relocation
    operands or destinations in the gap; never trim or rewrite the payload.
    """
    gap = start + size
    tail = payload[gap:end]
    lea = (len(tail) == 3 and tail[0] == 0x8d and tail[2] == 0
           and tail[1] >> 6 == 1 and (tail[1] & 7) != 4)
    mov = len(tail) == 2 and tail[0] == 0x8b and tail[1] >> 6 == 3
    if not (lea or mov) or ((tail[1] >> 3) & 7) != (tail[1] & 7):
        return False
    if end not in member_starts or end % 4 or (rva + end - start) % 4:
        return False
    local_symbols = {obj.sym_name(index): value
                     for index, value, owner in obj.iter_symbols()
                     if owner == section}
    for sec in obj.section_table:
        raw = obj.section_payload(sec['index'])
        for site, (name, kind) in obj.typed_relocations(sec['index']).items():
            if sec['index'] == section and site < end and site + 4 > gap:
                return False
            if name in local_symbols:
                # The donor includes DIR32NB/SECREL debug references when
                # checking whether a section's padding is addressed.
                if kind not in (6, 7, 11, 20) or not 0 <= site <= len(raw) - 4:
                    return False
                destination = local_symbols[name] + struct.unpack_from('<i', raw, site)[0]
                if gap <= destination < end:
                    return False
    tail_rva = rva + size
    if image.relocs_in(tail_rva - 3, tail_rva + len(tail)):
        return False
    return image.pe.read(tail_rva, len(tail)) == tail


def _string_extents(model):
    return {binding.rva: binding.size for binding in model.data
            if binding.kind == 'string' and binding.size > 0}


def _admitted_string_pairing(candidate, retail, refs, found, members, model, image):
    """HoMM1 d3c9297 _paired_votes, bounded by admitted strings and instructions.

    Other instructions may differ; every actual typed operand instruction must
    have the same boundary, encoding and field. Nonliteral referents are already
    independently checked by the caller. This proves no function score.
    """
    if not found or not isinstance(members, dict):
        return []
    admitted = _string_extents(model)
    eligible = []
    for name, rva in found:
        entry = members.get(name)
        if not isinstance(entry, tuple) or len(entry) < 2:
            continue
        payload = entry[1]
        if not isinstance(payload, bytes) or not payload.endswith(b'\0'):
            continue
        if payload.find(b'\0') != len(payload) - 1 or admitted.get(rva) != len(payload):
            continue
        if image.pe.read(rva, len(payload)) == payload:
            eligible.append((name, rva))
    if not eligible:
        return []
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    decoder.detail = True
    def fields(blob):
        result = {}
        instructions = list(decoder.disasm(bytes(blob), 0))
        if sum(i.size for i in instructions) != len(blob):
            return None
        for ins in instructions:
            for off, width in ((ins.imm_offset, ins.imm_size),
                               (ins.disp_offset, ins.disp_size)):
                if width == 4:
                    result[ins.address + off] = (ins.address, ins.size, off)
        return result
    mine, theirs = fields(candidate), fields(retail)
    if mine is None or theirs is None:
        return []
    for off in refs:
        if off not in mine or mine[off] != theirs.get(off):
            return []
        start, size, _ = mine[off]
        if candidate[start:start + size] != retail[start:start + size]:
            return []
    return eligible


def referenced_addresses(model, image, unit, obj, members, *, x87_widths=None, admitted_strings=False):
    addresses_by_name = defaultdict(set)
    for binding in model.functions + model.data:
        if binding.channel and binding.name:
            addresses_by_name[mask(binding.name)].add(binding.rva)
    known = {name: next(iter(values)) for name, values in addresses_by_name.items()
             if len(values) == 1}
    # TU-local names can repeat across units: this object's own declaration
    # takes precedence over an unrelated unit's same-spelled static.
    known.update({mask(b.name): b.rva for b in model.functions + model.data
                  if b.channel and b.name and b.unit == unit})
    code_extents = {(b.unit, mask(b.name)): (b.rva, b.size) for b in model.functions
                    if b.channel and b.unit and b.name and b.size}
    data_extents = {(b.unit, mask(b.name)): (b.rva, b.size) for b in model.data
                    if b.channel and b.unit and b.name and getattr(b, 'size', 0)}
    addresses = defaultdict(set)
    for sec in obj.section_table:
        code = bool(sec['characteristics'] & 0x20000000)
        if not code and sec['name'] not in ('.data', '.rdata'):
            continue
        extents = code_extents if code else data_extents
        symbols = obj.defined_symbols(sec['index'])
        # Real VC6 TU-local functions and source globals have class STATIC.
        # Only source-owned extents may act as anchors; local labels cannot.
        if hasattr(obj, 'section_members'):
            symbols = sorted(set(symbols) | {(offset, name) for offset, name, _scl
                            in obj.section_members(sec['index'])
                            if (unit, mask(name)) in extents})
        starts = sorted({offset for offset, _ in symbols})
        member_starts = ({offset for offset, _name, _scl
                          in obj.section_members(sec['index'])}
                         if code and hasattr(obj, 'section_members') else set())
        payload = obj.section_payload(sec['index'])
        typed = obj.typed_relocations(sec['index'])
        for start, function in symbols:
            extent = extents.get((unit, mask(function)))
            if extent is None:
                continue
            rva, size = extent
            end = next((offset for offset in starts if offset > start), len(payload))
            # Local labels inside the claimed body remain code labels. A
            # label at/after its end may start an appended VC6 switch table.
            end = min(end, min((offset for offset in member_starts
                                if offset >= start + size), default=end))
            if start + size > end:
                continue
            if (code and any(b not in (0x90, 0xcc) for b in payload[start + size:end])
                    and not _exact_instruction_alignment_tail(
                        obj, sec['index'], payload, start, size, end,
                        rva, member_starts, image)):
                continue
            candidate = bytearray(payload[start:start + size])
            raw = image.pe.read(rva, size)
            if raw is None:
                continue
            retail = bytearray(raw)
            refs = {offset - start: value for offset, value in typed.items() if start <= offset < start + size}
            absolute = {off for off, (name, kind) in refs.items() if kind == 6 and name != '__except_list'}
            if absolute != {site - rva for site in image.relocs_in(rva, rva + size)}:
                continue
            valid, found, all_known = True, [], True
            for off, (name, kind) in refs.items():
                if off + 4 > size or kind not in (6, 20):
                    valid = False
                    break
                addend = struct.unpack_from('<i', candidate, off)[0]
                if kind == 6 and name != '__except_list':
                    value = struct.unpack_from('<I', retail, off)[0] - image.image_base
                    if name in members:
                        found.append((name, value - addend))
                    anchor = known.get(mask(name))
                    if anchor is None and name not in members:
                        all_known = False
                    if anchor is not None and value != anchor + addend:
                        valid = False
                        break
                elif kind == 20:
                    anchor = known.get(mask(name))
                    if anchor is None and name not in members:
                        all_known = False
                    value = rva + off + 4 + struct.unpack_from('<i', retail, off)[0]
                    if anchor is not None and value != anchor + addend:
                        valid = False
                        break
                candidate[off:off + 4] = retail[off:off + 4] = bytes(4)
            # The donor FP-pool oracle pairs reviewed operands independently
            # of whole-function matching. For real constants only, an exact
            # direct-memory x87 read also proves the operand's type and width.
            # Other instruction differences remain in the ordinary comparison.
            # This does not apply to strings, globals, or arbitrary pointers.
            fp_reads = bool(valid and candidate != retail and x87_widths and found)
            if fp_reads:
                uses = [(off, name, kind) for off, (name, kind) in refs.items()
                        if name in members]
                fp_reads = all(kind == 6 and off >= 2 and
                               candidate[off - 2:off] == retail[off - 2:off]
                               for off, name, kind in uses)
                if fp_reads:
                    # Reuse the donor decoder so opcode-like bytes inside
                    # another instruction cannot establish a read boundary.
                    from hobbit.delink.data_manifest import _fp_read_widths
                    candidate_reads, retail_reads = _fp_read_widths(
                        [payload[start:start + size], raw])
                    fp_reads = all(candidate_reads.get(off) == retail_reads.get(off) == x87_widths[name]
                                   for off, name, _kind in uses)
            paired_strings = []
            if valid and code and admitted_strings and all_known and candidate != retail:
                paired_strings = _admitted_string_pairing(
                    candidate, retail, refs, found, members, model, image)
            proved = found if valid and (candidate == retail or fp_reads) else paired_strings
            for name, address in proved:
                addresses[name].add(address)
    return addresses
