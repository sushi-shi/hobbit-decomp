"""hobbit.walls.pairscan - the shared normalized-pair substrate for sieves.

The sieves (aggregate-copies, eh-frame, global-refs) all read the SAME
evidence objdiff scored: the normalized comparison copies under
build/objdiff/compare-new/{base,target}, via delink.coffx topology and
tool.objdump decoding. One module owns the function-window rule (a window is
cut at the NEXT DEFINED SYMBOL in its own section; cl's `$L<n>`/`$name$<n>`
block labels belong to the enclosing COMDAT and are never boundaries) and
the `$S<hash>` / `??_E` canonicalisation, so the code side and the data side
cannot drift apart.
"""

from __future__ import annotations

import re
import struct
from pathlib import Path

from hobbit.core.paths import BUILD
from hobbit.core.msvc_names import anonymous_namespaces
from hobbit.delink.coffx import Obj

NORM = BUILD / "objdiff/compare-new"

MEM_EXECUTE = 0x20000000
DIR32, REL32 = 0x06, 0x14

LOCAL_LABEL = re.compile(r"^\$(?:L\d+|\w+\$\d+)$")
# the three spellings one TU-local static reaches a join under: canonicalized
# (`$Sdata_data_<digest>_<n>`), cl's own CodeView counter, and the Model's
# ordinal-free name - bare `$S`, or `$S<rva>` where several units share a name.
LOCAL_STATIC_SUFFIX = re.compile(r"\$S(?:data_data_[0-9a-f]+_[0-9]+|[0-9]*)$")
_VDTOR = re.compile(r"^\?\?_E")
COMPGEN = re.compile(r"^_?\$[ES][0-9]+$|^\$anon_(?:data|f32|f64)_[0-9a-f]+_[0-9]+$")


def is_local_label(name: str) -> bool:
    return bool(LOCAL_LABEL.match(name))


def canon(name: str) -> str:
    """Strip named-static suffixes and fold the weak vector-deleting alias.

    A bare compiler ordinal is an identity, not a named static's suffix:
    stripping ``_$S56`` to ``_`` aliases a real source datum named ``_``.
    """
    name = anonymous_namespaces(name)
    if COMPGEN.fullmatch(name):
        return name
    return _VDTOR.sub("??_G", LOCAL_STATIC_SUFFIX.sub("", name))


def pairs(units=None) -> dict[str, tuple[Path, Path]]:
    """{unit: (base_obj, target_obj)} for every normalized pair on disk."""
    out = {}
    for base in sorted((NORM / "base").glob("*.obj")):
        unit = base.stem
        if units and unit not in units:
            continue
        target = NORM / "target" / f"{unit}.c.obj"
        if target.is_file():
            out[unit] = (base, target)
    return out


def require_pairs(units=None) -> dict[str, tuple[Path, Path]]:
    """`pairs()`, but an EMPTY result is an ERROR, not an answer. A sieve that
    prints '0 mismatches' from an unbuilt tree reads as a clean run."""
    import sys
    found = pairs(units)
    if not found:
        print(f"[walls] no normalized base/target pair under {NORM}"
              + (f" for {', '.join(sorted(units))}" if units else "")
              + " - run `hobbit build` (or `hobbit compare`) first",
              file=sys.stderr)
        raise SystemExit(2)
    return found


def scores():
    """{(unit, symbol): fuzzy%} + the live unit set, from the report."""
    from hobbit.walls.inventory import report_scores
    _path, sc = report_scores()
    return sc, {u for (u, _s) in sc}


def functions(obj: Obj) -> dict[str, tuple[int, int, int]]:
    """{symbol: (secnum, start, end)} for every function body in the object.
    `end` = the next NON-LABEL defined symbol in the section, or its size."""
    out: dict[str, tuple[int, int, int]] = {}
    for secnum in range(1, obj.nsec + 1):
        sec = obj.section_table[secnum - 1]
        if not sec["characteristics"] & MEM_EXECUTE:
            continue
        members = [(v, n) for v, n, _s in obj.section_members(secnum)
                   if not is_local_label(n)]
        size = len(obj.section_payload(secnum)) or sec["size"]
        for i, (val, nm) in enumerate(members):
            end = members[i + 1][0] if i + 1 < len(members) else size
            out[nm] = (secnum, val, end)
    return out


def fn_relocs(obj: Obj, secnum: int, lo: int, hi: int):
    """[(offset, name, type, addend)] inside one window, addend read from the
    section payload bytes at the site (COFF stores it in the field itself)."""
    payload = obj.section_payload(secnum)
    out = []
    for off, (name, typ) in obj.typed_relocations(secnum).items():
        if not lo <= off < hi:
            continue
        addend = struct.unpack_from("<I", payload, off)[0] \
            if off + 4 <= len(payload) else 0
        out.append((off, name, typ, addend))
    out.sort()
    return out



def function_body(obj: Obj, secnum: int, lo: int, hi: int) -> bytes:
    """Keep operands and inline data; remove only established terminal padding."""
    body = obj.section_payload(secnum)[lo:hi]
    for index, value, section in obj.iter_symbols():
        offset = obj.symptr + index * 18
        typ, storage, auxiliaries = struct.unpack_from("<HBB", obj.buf, offset + 14)
        if section == secnum and value == lo and typ == 0x20 and storage in (2, 3):
            if auxiliaries:
                size = struct.unpack_from("<I", obj.buf, offset + 18 + 4)[0]
                if 0 < size <= len(body):
                    body = body[:size]
                    break
    if not body or body[-1] not in (0x90, 0xcc):
        return body

    relocations = {site - lo: target for site, target in obj.typed_relocations(secnum).items()
                   if lo <= site < hi}
    local_addresses = {name: value - lo for value, name, _storage
                       in obj.section_members(secnum)}
    local_refs = {site: local_addresses[name] + struct.unpack_from("<i", body, site)[0]
                  for site, (name, typ) in relocations.items()
                  if typ == DIR32 and name in local_addresses and site + 4 <= len(body)}
    local_refs = {site: target for site, target in local_refs.items()
                  if 0 <= target < len(body)}
    table_end = None
    code_end = len(body)
    if local_refs:
        # Terminal pointer tables have a contiguous self-relocated entry run,
        # point back into code, and are themselves addressed by earlier code.
        # Selector tails need the switch decoder's independently proven bound.
        last = max(local_refs)
        first = last
        while first - 4 in local_refs:
            first -= 4
        candidate_end = last + 4
        if (first == last or candidate_end >= len(body)
                or not any(site < first and target == first
                           for site, target in local_refs.items())
                or any(not 0 <= local_refs[site] < first
                       for site in range(first, candidate_end, 4))):
            return body
        table_end, code_end = candidate_end, first
    from hobbit.tool import objdump
    decoded = []
    for line in objdump.disassemble(body[:code_end]).splitlines():
        if ":\t" not in line:
            continue
        address, rest = line.split(":\t", 1)
        fields = rest.split("\t")
        if len(fields) < 2 or not fields[-1].strip():
            continue
        assembly = fields[-1].strip().split(None, 1)
        if assembly[0].startswith(".") or assembly[0] == "(bad)":
            return body
        decoded.append((int(address.strip(), 16), assembly[0],
                        assembly[1] if len(assembly) > 1 else ""))
    if table_end is not None:
        if any(target >= table_end for target in local_refs.values()):
            # Reuse the established switch bound analysis; never infer a
            # selector's length by stripping byte values that may be indices.
            from hobbit.walls.semdiff import Line
            from hobbit.walls.switchmap import switches
            lines = [Line(address, mnemonic + (" " + operands if operands else ""), None)
                     for address, mnemonic, operands in decoded]
            bounded = [switch for switch in switches(body, lines, depth=0)
                       if switch.table == code_end and switch.index == table_end
                       and switch.bound is not None]
            if len(bounded) != 1:
                return body
            selector_end = table_end + bounded[0].bound + 1
            if (selector_end > len(body)
                    or any(index >= (table_end - code_end) // 4
                           for index in body[table_end:selector_end])
                    or any(target >= selector_end for target in local_refs.values())):
                return body
            table_end = selector_end
        if any(byte not in (0x90, 0xcc) for byte in body[table_end:]):
            return body
    end = table_end if table_end is not None else len(body)
    remaining = len(decoded)
    if table_end is None:
        while remaining:
            address, mnemonic, _operands = decoded[remaining - 1]
            if mnemonic not in ("nop", "int3") or end - address != 1:
                break
            end = address
            remaining -= 1
        if end == len(body) or not remaining:
            return body
        terminal = decoded[remaining - 1][1]
        if not (terminal.startswith("ret") or terminal == "jmp"):
            return body
    for _address, mnemonic, operands in decoded[:remaining]:
        if mnemonic.startswith(("j", "loop", "call")):
            target = re.fullmatch(r"0x([0-9a-f]+)", operands)
            if target and end <= int(target[1], 16) < len(body):
                return body
    # Relocation fields, including addend bytes, must remain fully represented.
    widths = {1: 2, 2: 2, 6: 4, 7: 4, 9: 2, 10: 2, 11: 4, 12: 4, 13: 1, 20: 4}
    if any(typ not in widths or site + widths[typ] > end
           for site, (_name, typ) in relocations.items()):
        return body
    if any(lo + end <= value < hi for value, _name, _storage
           in obj.section_members(secnum)):
        return body
    addresses = {name: value for value, name, _storage in obj.section_members(secnum)}
    for source_section in range(1, obj.nsec + 1):
        source = obj.section_payload(source_section)
        for site, (name, typ) in obj.typed_relocations(source_section).items():
            if name not in addresses:
                continue
            if typ not in (DIR32, 7, 11, REL32):
                return body
            if site + 4 > len(source):
                return body
            destination = addresses[name] + struct.unpack_from("<i", source, site)[0]
            if lo + end <= destination < hi:
                return body
    return body[:end]


def insns(obj: Obj, secnum: int, lo: int, hi: int, *, intel: bool = True):
    """[(offset, mnemonic, operands)] for one window, via tool.objdump.
    Offsets are window-relative; only established padding is excluded."""
    from hobbit.tool import objdump
    body = function_body(obj, secnum, lo, hi)
    if not body:
        return []
    text = objdump.disassemble(body, vma=0, intel=intel)
    out = []
    for line in text.splitlines():
        if ":\t" not in line:
            continue
        addr, rest = line.split(":\t", 1)
        parts = rest.split("\t")
        if len(parts) < 2:
            continue
        asm = parts[-1].strip()
        if not asm or asm.startswith("."):
            continue
        toks = asm.split(None, 1)
        mn = toks[0]
        ops = toks[1] if len(toks) > 1 else ""
        # fold prefixes the sieves care about into the mnemonic
        if mn in ("rep", "repz", "repnz", "repe", "repne", "lock") and ops:
            t2 = ops.split(None, 1)
            mn = f"{mn} {t2[0]}"
            ops = t2[1] if len(t2) > 1 else ""
        try:
            out.append((int(addr.strip(), 16), mn, ops))
        except ValueError:
            continue
    return out
