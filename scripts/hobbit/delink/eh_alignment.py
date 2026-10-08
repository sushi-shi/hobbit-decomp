"""Candidate EH alignment after full owner-proved bytes/reference replay.

Uses local Gruntz 7d4bd55b99e32f084834d991badf7609889481f4's structural
EH naming and COFF alignment; preserves all unproved legacy rows unchanged.
"""
import struct

from hobbit.compare.canonicalize import (
    CoffObject, _eh_funcinfo_owners, _eh_funclet_owners,
)
from hobbit.delink.coffx import section_alignment
from hobbit.delink.data_manifest import member_alignment


def prove(raw, rows, known, read, absolute_sites):
    coff = CoffObject(raw)
    names = _eh_funcinfo_owners(coff)
    names.update(_eh_funclet_owners(coff))
    by_name = {}
    for index, name in names.items():
        by_name.setdefault(name, []).append(coff.symbols[index])
    proofs = {}
    for row in rows:
        hits = by_name.get(row['name'], [])
        if len(hits) != 1:
            continue
        symbol = hits[0]
        if not 0 < symbol.section <= len(coff.sections):
            continue
        section = coff.sections[symbol.section - 1]
        size, rva, offset = row['size'], row['rva'], symbol.value
        if (size <= 0 or section.name != '.xdata$x' or not section.raw_offset
                or offset < 0 or offset + size > section.raw_size
                or section.raw_offset + section.raw_size > len(coff.data)):
            continue
        if section.characteristics & 0x20000000 or not section.characteristics & 0x40:
            continue
        stored = coff.section_bytes(section)
        data = bytearray(stored[offset:offset + size])
        expected = []
        valid = True
        for relocation in coff.relocations:
            if (relocation.section != symbol.section
                    or relocation.site + 4 <= offset or relocation.site >= offset + size):
                continue
            if relocation.typ != 6 or relocation.site < offset or relocation.site + 4 > offset + size:
                valid = False
                break
            target = coff.symbols[relocation.symbol_index]
            name = names.get(target.index, target.name)
            if name not in known:
                valid = False
                break
            addend = struct.unpack_from('<I', data, relocation.site - offset)[0]
            value = known[name] + addend
            struct.pack_into('<I', data, relocation.site - offset, value & 0xffffffff)
            expected.append(relocation.site - offset)
        if not valid or bytes(data) != read(rva, size):
            continue
        if sorted(expected) != [site - rva for site in absolute_sites if rva <= site < rva + size]:
            continue
        alignment = section_alignment(section.characteristics)
        if alignment <= 0 or rva % alignment != offset % alignment:
            continue
        proofs[row['name']] = dict(
            alignment=member_alignment(alignment, offset, rva),
            section_alignment=alignment, section_offset=offset,
            section_size=section.raw_size, reference_offsets=expected,
        )
    return proofs
