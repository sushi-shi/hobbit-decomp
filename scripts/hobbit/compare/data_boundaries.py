"""Canonicalize proved one-past data addresses in disposable COFF copies.

A linked address at an object boundary cannot distinguish `first + sizeof
first` from `next + 0`. The enrolled data manifest and the object's own layout
must both prove that boundary before a DIR32 reference adopts the successor's
name. Definitions, symbol identities and all other offsets remain distinct.
"""

from __future__ import annotations

from collections import defaultdict
from dataclasses import dataclass
import struct

from hobbit.compare.canonicalize import CoffObject, DIR32, MEM_EXECUTE, MEM_WRITE
from hobbit.core import msvc_names


@dataclass(frozen=True)
class BoundaryRewrite:
    section: int
    site: int
    original_name: str
    original_addend: int
    canonical_name: str
    retail_rva: int
    resolved_offset: int


def canonicalize_boundaries(payload: bytes, object_name: str, rows: list[dict]):
    """Return (copy, rewrites); missing/ambiguous extent evidence changes nothing."""
    coff = CoffObject(payload)
    claims = [r for r in rows if r.get("object") == object_name]
    by_name, by_start = defaultdict(list), defaultdict(list)
    for row in claims:
        by_name[msvc_names.mask(row["name"])].append(row)
        by_start[row["rva"]].append(row)
    symbols = defaultdict(list)
    for symbol in coff.symbols.values():
        if symbol.section > 0 and symbol.typ == 0 and symbol.storage_class in (2, 3):
            section = coff.sections[symbol.section - 1]
            if not section.characteristics & MEM_EXECUTE:
                symbols[msvc_names.mask(symbol.name)].append(symbol)

    def section_record(symbol, section):
        # Ordinary COFF section metadata is not a second data identity. No
        # other auxiliary-bearing definition is exempt from alias detection.
        if (symbol.name != section.name or symbol.value != 0
                or symbol.typ != 0 or symbol.storage_class != 3
                or symbol.aux_count != 1):
            return False
        extent, relocations, lines = struct.unpack_from("<IHH", payload, symbol.offset + 18)
        line_count = struct.unpack_from("<H", payload, section.header_offset + 34)[0]
        return (extent, relocations, lines) == (section.raw_size, section.reloc_count, line_count)

    def definition(row):
        ordinal = row.get("section_ordinal", "-")
        if type(ordinal) not in (int, str):
            return None
        try:
            ordinal = int(ordinal)
        except ValueError:
            return None
        if (len(by_name[msvc_names.mask(row["name"])]) != 1
                or row.get("provenance") != "src-DATA-sizeof"
                or row["size"] <= 0 or row.get("storage") not in ("data", "rdata")
                or row["rva"] < 0 or row["rva"] + row["size"] > 0x100000000
                or ordinal <= 0
                or row.get("section_offset", "-") == "-"):
            return None
        matches = symbols.get(msvc_names.mask(row["name"]), ())
        if len(matches) != 1 or matches[0].aux_count != 0:
            return None
        symbol = matches[0]
        section = coff.sections[symbol.section - 1]
        storage = "data" if section.characteristics & MEM_WRITE else "rdata"
        if (section.raw_offset == 0 or storage != row["storage"]
                or symbol.value != row["section_offset"]
                or symbol.value + row["size"] > section.raw_size):
            return None
        # Neither a retail range nor its candidate definition may have an
        # overlapping claim/alias that makes ownership of the boundary unclear.
        if any(other is not row and other["rva"] < row["rva"] + row["size"]
               and row["rva"] < other["rva"] + other["size"] for other in claims):
            return None
        if any(other.index != symbol.index and other.section == symbol.section
               and symbol.value <= other.value < symbol.value + row["size"]
               and not section_record(other, section) for other in coff.symbols.values()):
            return None
        return symbol

    boundaries = {}
    for first in claims:
        successors = by_start.get(first["rva"] + first["size"], ())
        if len(successors) != 1:
            continue
        second = successors[0]
        if (first.get("storage") != second.get("storage")
                or first.get("section_ordinal") != second.get("section_ordinal")):
            continue
        a, b = definition(first), definition(second)
        if (a is None or b is None or a.section != b.section
                or a.value + first["size"] != b.value):
            continue
        boundaries[a.index] = (b, first["size"], second["rva"])

    data, rewrites = bytearray(payload), []
    for relocation in coff.relocations:
        boundary = boundaries.get(relocation.symbol_index)
        if relocation.typ != DIR32 or boundary is None:
            continue
        successor, size, rva = boundary
        section = coff.sections[relocation.section - 1]
        if section.raw_offset == 0 or relocation.site + 4 > section.raw_size:
            continue
        offset = section.raw_offset + relocation.site
        addend = struct.unpack_from("<I", payload, offset)[0]
        if addend != size:
            continue
        if any(other.offset != relocation.offset and other.section == relocation.section
               and abs(other.site - relocation.site) < 4 for other in coff.relocations):
            continue
        original = coff.symbols[relocation.symbol_index]
        struct.pack_into("<I", data, relocation.offset + 4, successor.index)
        struct.pack_into("<I", data, offset, 0)
        rewrites.append(BoundaryRewrite(relocation.section, relocation.site,
                                        original.name, addend, successor.name,
                                        rva, successor.value))

    result = bytes(data)
    after = CoffObject(result)
    expected = bytearray(payload)
    changed = {(r.section, r.site): r for r in rewrites}
    if (after.sections != coff.sections or after.symbols != coff.symbols
            or len(after.relocations) != len(coff.relocations)):
        raise RuntimeError("data-boundary normalization changed COFF topology")
    for before, current in zip(coff.relocations, after.relocations):
        rewrite = changed.get((before.section, before.site))
        if rewrite is None:
            if before != current:
                raise RuntimeError("data-boundary normalization changed an unrelated relocation")
            continue
        target = after.symbols[current.symbol_index]
        original = coff.symbols[before.symbol_index]
        section = coff.sections[before.section - 1]
        offset = section.raw_offset + before.site
        addend = struct.unpack_from("<I", result, offset)[0]
        if ((before.offset, before.section, before.site, before.typ)
                != (current.offset, current.section, current.site, current.typ)
                or target.name != rewrite.canonical_name or addend != 0
                or (original.section, original.value + rewrite.original_addend)
                != (target.section, target.value + addend)):
            raise RuntimeError("data-boundary normalization changed a resolved address")
        struct.pack_into("<I", expected, before.offset + 4, target.index)
        struct.pack_into("<I", expected, offset, 0)
    if result != bytes(expected):
        raise RuntimeError("data-boundary normalization changed unrelated bytes")
    return result, tuple(rewrites)


def sidecar_bytes(base, target):
    lines = ["side\tsection\tsite\toriginal_name\toriginal_addend\tcanonical_name"
             "\tretail_rva\tresolved_section_offset\tproof"]
    for side, rewrites in (("base", base), ("target", target)):
        lines += [f"{side}\t{r.section}\t0x{r.site:x}\t{r.original_name}"
                  f"\t0x{r.original_addend:x}\t{r.canonical_name}\t0x{r.retail_rva:x}"
                  f"\t0x{r.resolved_offset:x}\tmanifest-and-COFF-adjacent-one-past-address"
                  for r in rewrites]
    return ("\n".join(lines) + "\n").encode("utf-8")
