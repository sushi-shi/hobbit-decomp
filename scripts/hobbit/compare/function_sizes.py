"""Prove paired function extents without changing their section payloads.

COFF normally states no function size. Objdiff infers it by decoding through
the next symbol, which can retain alignment NOPs as bytes of a switch selector
array. When a single-function COMDAT differs from its paired window only by
unreferenced terminal alignment NOPs, state the complete shorter window as a
COFF function auxiliary TotalSize on both disposable comparison copies.

VC6 can also place a two- or three-byte no-op between the final RET and an inline
jump table. A bounded table/dispatch proof permits the same metadata when
the complete paired body agrees; all alignment and table bytes stay intact.

The same exact paired-window proof states sizes for table-owned EH action
labels. VC6 emits those as class LABEL without a function type; an ordinary
branch label inside them otherwise truncates objdiff's inferred extent.
Only such proved EH entries receive function metadata. Interior labels,
section payloads, and relocation identities remain intact.
"""

from __future__ import annotations

from collections import Counter
from dataclasses import dataclass
import struct
import re

from hobbit.compare.canonicalize import (
    CoffObject, DIR32, EXTERNAL_STORAGE, FUNCTION_TYPE, MEM_EXECUTE,
    RELOCATION_WIDTHS, SYMBOL_SIZE, WEAK_EXTERNAL_STORAGE,
)
from hobbit.delink.eh_band import is_band_symbol

LNK_COMDAT = 0x1000
FILE_STORAGE = 103


@dataclass(frozen=True)
class SizeProof:
    name: str
    base_span: int
    target_span: int
    size: int
    alignment: int
    proof: str = 'paired-identical-window-with-terminal-alignment-nops'


def _eh_label(symbol):
    return symbol.storage_class == 6 and symbol.typ == 0 and is_band_symbol(symbol.name)


def _windows(coff: CoffObject):
    functions = [s for s in coff.symbols.values()
                 if s.section > 0 and ((s.typ == FUNCTION_TYPE
                 and s.storage_class == EXTERNAL_STORAGE) or _eh_label(s)) and s.aux_count == 0
                 and coff.sections[s.section - 1].characteristics & MEM_EXECUTE]
    counts = Counter(s.name for s in functions)
    result = {}
    for symbol in functions:
        if counts[symbol.name] != 1:
            continue
        section = coff.sections[symbol.section - 1]
        # Local block/switch labels stay inside the enclosing function.
        end = min((s.value for s in coff.symbols.values()
                   if s.section == symbol.section and s.value > symbol.value
                   and (s.storage_class in (2, 3) or _eh_label(s)) and s.aux_count == 0),
                  default=section.raw_size)
        if 0 <= symbol.value < end <= section.raw_size:
            result[symbol.name] = (symbol, end - symbol.value)
    return result


def _relocations(coff, symbol, size):
    return [(r.site - symbol.value, r.typ, coff.symbols[r.symbol_index].name)
            for r in coff.relocations
            if r.section == symbol.section
            and symbol.value <= r.site < symbol.value + size]


def _padding_alignment(coff, symbol, span, size):
    section = coff.sections[symbol.section - 1]
    nibble = (section.characteristics >> 20) & 15
    if not 1 <= nibble <= 14:
        return None
    alignment = 1 << (nibble - 1)
    if (not section.characteristics & LNK_COMDAT or symbol.value != 0
            or span != section.raw_size or not 0 < span - size < alignment
            or span % alignment):
        return None
    functions = [s for s in coff.symbols.values()
                 if s.section == symbol.section and s.typ == FUNCTION_TYPE]
    if len(functions) != 1:
        return None
    if coff.section_bytes(section)[size:] != b"\x90" * (span - size):
        return None
    if any(s.section == symbol.section and s.value >= size
           for s in coff.symbols.values()):
        return None
    for relocation in coff.relocations:
        width = RELOCATION_WIDTHS.get(relocation.typ)
        if relocation.section == symbol.section:
            if width is None or relocation.site + width > size:
                return None
        target = coff.symbols[relocation.symbol_index]
        if target.section == symbol.section and relocation.typ in (DIR32, 0x7, 0xB, 0x14):
            owner = coff.sections[relocation.section - 1]
            addend = struct.unpack_from("<i", coff.section_bytes(owner),
                                        relocation.site)[0]
            if size <= target.value + addend <= span:
                return None
    return alignment


def _pretable_padding_alignment(coff, symbol, span, size):
    """VC6's register-preserving no-op before a four-byte local jump table.

    This only states the paired function's complete extent. The no-op, all
    table/selector bytes, and every relocation stay in their original section.
    """
    section = coff.sections[symbol.section - 1]
    raw = coff.section_bytes(section)
    tail = raw[size:span]
    lea = tail == b"\x8d\x49\x00"
    mov = (len(tail) == 2 and tail[0] == 0x8b and tail[1] >> 6 == 3
           and ((tail[1] >> 3) & 7) == (tail[1] & 7))
    terminal_return = raw[size - 1:size] == b"\xc3"
    possible_ret = size >= 3 and raw[size - 3] == 0xc2
    possible_jump = ((size >= 2 and raw[size - 2] == 0xeb)
                     or (size >= 5 and raw[size - 5] == 0xe9))
    if (lea or mov) and (possible_ret or possible_jump):
        # Prove the final instruction boundary, not opcode-like bytes inside
        # an operand. A local unconditional jump also prevents fallthrough
        # into alignment, but its destination must be real paired code.
        from hobbit.sema.disasm import parse_listing
        from hobbit.tool.objdump import disassemble
        instructions = parse_listing(disassemble(raw[:size]))
        decoded = (bool(instructions) and instructions[0].rva == 0
                   and instructions[-1].end == size
                   and b"".join(i.raw for i in instructions) == raw[:size]
                   and all(a.end == b.rva for a, b in
                           zip(instructions, instructions[1:])))
        terminal_return = False
        if decoded:
            last = instructions[-1]
            terminal_return = (possible_ret and len(last.raw) == 3
                               and last.raw[0] == 0xc2 and last.mnemonic == "ret")
            if (possible_jump and last.mnemonic == "jmp"
                    and last.raw[0] in (0xeb, 0xe9)
                    and last.branch_target() in {i.rva for i in instructions}
                    and not any(r.section == symbol.section
                                and r.site < size
                                and r.site + RELOCATION_WIDTHS.get(r.typ, 4) > last.rva
                                for r in coff.relocations)):
                terminal_return = True
    if (not section.characteristics & LNK_COMDAT or symbol.value != 0
            or span >= section.raw_size or span % 4
            or not terminal_return or not (lea or mov)):
        return None
    functions = [s for s in coff.symbols.values()
                 if s.section == symbol.section and s.typ == FUNCTION_TYPE]
    if len(functions) != 1:
        return None
    local_tables = [s for s in coff.symbols.values()
                    if s.section == symbol.section and s.typ == 0
                    and s.storage_class == 3 and s.aux_count == 0
                    and re.fullmatch(r"\$L[0-9]+", s.name)]
    if not any(s.value == span for s in local_tables):
        return None
    end = min((s.value for s in local_tables if s.value > span),
              default=section.raw_size)
    if not span < end <= section.raw_size or (end - span) % 4:
        return None
    if any(s.section == symbol.section and size <= s.value < span
           for s in coff.symbols.values()):
        return None
    by_site = {r.site: r for r in coff.relocations if r.section == symbol.section}
    if end == section.raw_size:
        table_end = span
        while table_end in by_site:
            table_end += 4
        # An unlabelled final table can precede ordinary terminal COMDAT
        # NOP alignment. Reuse the existing complete suffix/reference proof.
        if (span < table_end < end and
                _padding_alignment(coff, symbol, end, table_end) is not None):
            end = table_end
    if {site for site in by_site if span <= site < end} != set(range(span, end, 4)):
        return None
    for offset in range(span, end, 4):
        relocation = by_site.get(offset)
        if (relocation is None or relocation.typ != DIR32
                or relocation.symbol_index != symbol.index
                or not 0 <= struct.unpack_from("<I", raw, offset)[0] < size):
            return None
    indexed_jump = False
    for relocation in coff.relocations:
        target = coff.symbols[relocation.symbol_index]
        if relocation.section == symbol.section:
            width = RELOCATION_WIDTHS.get(relocation.typ)
            if width is None or (relocation.site < span
                                 and relocation.site + width > size):
                return None
        if target.section != symbol.section:
            continue
        if relocation.typ not in (DIR32, 0x7, 0xB, 0x14):
            return None
        owner = coff.sections[relocation.section - 1]
        addend = struct.unpack_from("<i", coff.section_bytes(owner), relocation.site)[0]
        destination = target.value + addend
        if size <= destination < span:
            return None
        if (relocation.section == symbol.section and relocation.typ == DIR32
                and relocation.site >= 3 and relocation.site + 4 <= size
                and destination == span
                and raw[relocation.site - 3:relocation.site - 1] == b"\xff\x24"):
            sib = raw[relocation.site - 1]
            if sib & 0xc7 == 0x85 and (sib >> 3) & 7 != 4:
                indexed_jump = True
    return 4 if indexed_jump else None


def _supports_auxiliary_remap(coff):
    """Accept only the auxiliary formats whose symbol-index fields we know."""
    if any(r.offset + 10 > coff.symbol_offset for r in coff.relocations):
        return False
    for section in coff.sections:
        if section.raw_offset and section.raw_offset + section.raw_size > coff.symbol_offset:
            return False
        pointer = struct.unpack_from("<I", coff.data, section.header_offset + 28)[0]
        lines = struct.unpack_from("<H", coff.data, section.header_offset + 34)[0]
        if lines and (pointer == 0 or pointer + lines * 6 > coff.symbol_offset):
            return False
    for symbol in coff.symbols.values():
        if not symbol.aux_count:
            continue
        if symbol.storage_class == FILE_STORAGE and symbol.value == 0:
            continue                         # filename bytes, no indices
        if symbol.aux_count != 1:
            return False
        if symbol.storage_class == WEAK_EXTERNAL_STORAGE:
            index = struct.unpack_from("<I", coff.data, symbol.offset + SYMBOL_SIZE)[0]
            if index not in coff.symbols:
                return False
            continue                         # TagIndex is its default
        if symbol.storage_class == EXTERNAL_STORAGE and symbol.typ == FUNCTION_TYPE:
            for field in (0, 12):
                index = struct.unpack_from("<I", coff.data,
                                           symbol.offset + SYMBOL_SIZE + field)[0]
                if index and index not in coff.symbols:
                    return False
            continue                         # TagIndex, PointerToNextFunction
        if symbol.storage_class == 3 and symbol.typ == 0 and symbol.section > 0:
            # COFF's section-aux format predicate, also used by object::read.
            # EH normalization can rename the section but leave this symbol.
            continue                         # section number is not a symbol index
        return False
    return True


def _state_sizes(coff, sizes):
    if not sizes:
        return coff.data
    remap, count = {}, 0
    for symbol in coff.symbols.values():
        remap[symbol.index] = count
        count += 1 + symbol.aux_count + (symbol.index in sizes)

    prefix = bytearray(coff.data[:coff.symbol_offset])
    struct.pack_into("<I", prefix, 12, count)
    for relocation in coff.relocations:
        struct.pack_into("<I", prefix, relocation.offset + 4,
                         remap[relocation.symbol_index])
    # A line-number record with LineNumber=0 contains a function symbol index.
    for section in coff.sections:
        pointer = struct.unpack_from("<I", prefix, section.header_offset + 28)[0]
        lines = struct.unpack_from("<H", prefix, section.header_offset + 34)[0]
        for offset in range(pointer, pointer + lines * 6, 6):
            if struct.unpack_from("<H", prefix, offset + 4)[0] == 0:
                old = struct.unpack_from("<I", prefix, offset)[0]
                struct.pack_into("<I", prefix, offset, remap[old])

    symbols = bytearray()
    for symbol in coff.symbols.values():
        record = bytearray(coff.data[symbol.offset:symbol.offset + SYMBOL_SIZE])
        start = symbol.offset + SYMBOL_SIZE
        aux = bytearray(coff.data[start:start + symbol.aux_count * SYMBOL_SIZE])
        fields = ()
        if symbol.aux_count and symbol.storage_class == WEAK_EXTERNAL_STORAGE:
            fields = (0,)
        elif (symbol.aux_count and symbol.storage_class == EXTERNAL_STORAGE
              and symbol.typ == FUNCTION_TYPE):
            fields = (0, 12)
        for field in fields:
            old = struct.unpack_from("<I", aux, field)[0]
            if old:
                struct.pack_into("<I", aux, field, remap[old])
        if symbol.index in sizes:
            if _eh_label(symbol):
                struct.pack_into('<HB', record, 14, FUNCTION_TYPE, EXTERNAL_STORAGE)
            record[17] = 1
            aux = struct.pack("<IIIIH", 0, sizes[symbol.index], 0, 0, 0)
        symbols += record + aux
    result = bytes(prefix + symbols + coff.data[coff.string_offset:])
    after = CoffObject(result)
    if (after.sections != coff.sections
            or after.symbol_count != count
            or len(after.relocations) != len(coff.relocations)
            or result[after.string_offset:] != coff.data[coff.string_offset:]):
        raise RuntimeError("function-size metadata changed section/string topology")
    for section in coff.sections:
        if coff.section_bytes(section) != after.section_bytes(section):
            raise RuntimeError("function-size metadata changed section payload")
    for before, current in zip(coff.relocations, after.relocations):
        if (before.section, before.site, before.typ, remap[before.symbol_index]) != (
                current.section, current.site, current.typ, current.symbol_index):
            raise RuntimeError("function-size metadata changed relocation identity")
    for before in coff.symbols.values():
        current = after.symbols[remap[before.index]]
        promoted = before.index in sizes and _eh_label(before)
        if (before.name, before.value, before.section,
                FUNCTION_TYPE if promoted else before.typ,
                EXTERNAL_STORAGE if promoted else before.storage_class,
                before.aux_count + (before.index in sizes)) != (
                current.name, current.value, current.section, current.typ,
                current.storage_class, current.aux_count):
            raise RuntimeError("function-size metadata changed symbol identity")
        if before.index in sizes:
            size = struct.unpack_from("<I", result, current.offset + SYMBOL_SIZE + 4)[0]
            if size != sizes[before.index]:
                raise RuntimeError("function-size metadata emitted the wrong extent")
        else:
            old_aux = bytearray(coff.data[before.offset + SYMBOL_SIZE:
                                         before.offset + (1 + before.aux_count) * SYMBOL_SIZE])
            fields = ()
            if before.aux_count and before.storage_class == WEAK_EXTERNAL_STORAGE:
                fields = (0,)
            elif (before.aux_count and before.storage_class == EXTERNAL_STORAGE
                  and before.typ == FUNCTION_TYPE):
                fields = (0, 12)
            for field in fields:
                index = struct.unpack_from("<I", old_aux, field)[0]
                if index:
                    struct.pack_into("<I", old_aux, field, remap[index])
            if old_aux != result[current.offset + SYMBOL_SIZE:
                                 current.offset + (1 + current.aux_count) * SYMBOL_SIZE]:
                raise RuntimeError("function-size metadata changed unrelated auxiliary evidence")
    return result


def paired_sizes(base: bytes, target: bytes):
    """Return (base copy, target copy, proofs); unsupported evidence is unchanged."""
    left, right = CoffObject(base), CoffObject(target)
    if not _supports_auxiliary_remap(left) or not _supports_auxiliary_remap(right):
        return base, target, ()
    mine, theirs = _windows(left), _windows(right)
    left_sizes, right_sizes, proofs = {}, {}, []
    for name in sorted(mine.keys() & theirs.keys()):
        a, a_span = mine[name]
        b, b_span = theirs[name]
        eh_extent = a_span == b_span and (_eh_label(a) or _eh_label(b))
        if a_span == b_span and not eh_extent:
            continue
        size = min(a_span, b_span)
        larger, symbol, span = (left, a, a_span) if a_span > b_span else (right, b, b_span)
        alignment = 1 if eh_extent else _padding_alignment(larger, symbol, span, size)
        proof = ('paired-identical-EH-map-owned-function' if eh_extent else
                 'paired-identical-window-with-terminal-alignment-nops')
        if alignment is None:
            alignment = _pretable_padding_alignment(larger, symbol, span, size)
            proof = 'paired-identical-window-with-pretable-alignment-nop'
        if alignment is None:
            continue
        a_bytes = left.section_bytes(left.sections[a.section - 1])[a.value:a.value + size]
        b_bytes = right.section_bytes(right.sections[b.section - 1])[b.value:b.value + size]
        if a_bytes != b_bytes or _relocations(left, a, size) != _relocations(right, b, size):
            continue
        left_sizes[a.index] = right_sizes[b.index] = size
        proofs.append(SizeProof(name, a_span, b_span, size, alignment, proof))
    return _state_sizes(left, left_sizes), _state_sizes(right, right_sizes), tuple(proofs)


def sidecar_bytes(proofs):
    lines = ["name\tbase_span\ttarget_span\tcompared_size\talignment\tproof"]
    lines += [f"{p.name}\t0x{p.base_span:x}\t0x{p.target_span:x}\t0x{p.size:x}"
              f"\t{p.alignment}\t{p.proof}"
              for p in proofs]
    return ("\n".join(lines) + "\n").encode("utf-8")
