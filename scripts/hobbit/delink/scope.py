"""Scope HoMM1's /FIXED manifest to complete objects selected for comparison.

The reviewed full-image inventory remains authoritative. A generated subset
includes EVERY admitted absolute field intersecting selected code/data ranges;
partial fields fail closed. No selection is made by the target's identity.
"""
from __future__ import annotations

import hashlib
import json
from pathlib import Path

from hobbit.core.paths import BUILD, RETAIL

OUTPUT = BUILD / 'gen/delink_absolute_relocations.tsv'
METADATA = BUILD / 'gen/delink_scope.json'


def select(sites: list[int], ranges: list[tuple[int, int]]) -> list[int]:
    import bisect
    chosen = set()
    for lo, hi in ranges:
        if hi <= lo:
            raise ValueError(f'invalid delink scope [{lo:#x}, {hi:#x})')
        # Include all intersecting fields, rejecting an incomplete selection.
        for site in sites[bisect.bisect_right(sites, lo - 4):bisect.bisect_left(sites, hi)]:
            if site < lo or site + 4 > hi:
                raise ValueError(f'absolute field {site:#x} crosses selected extent [{lo:#x}, {hi:#x})')
            chosen.add(site)
    return sorted(chosen)


def section_ranges(rows, sections):
    ranges = []
    seen = set()
    for section in sections:
        if section['rva'] is None:
            continue
        key = (section['object'], section['ordinal'])
        if section['ordinal'] <= 0 or key in seen:
            raise ValueError('invalid or duplicate section identity')
        seen.add(key)
        rva, size = section['rva'], section['size']
        if rva < 0 or size <= 0 or section['provenance'] != 'candidate-COFF-section':
            raise ValueError('invalid affine section')
        members = [row for row in rows if row['object'] == section['object']
                   and row.get('section_ordinal') == section['ordinal']]
        if not members:
            raise ValueError('affine section has no enrolled owner')
        for row in members:
            if row['storage'] != section['storage']:
                raise ValueError('section/member storage conflict')
            offset = row.get('section_offset')
            if (offset is None or offset < 0 or row['size'] <= 0
                    or offset + row['size'] > size or row['rva'] != rva + offset):
                raise ValueError('section/member evidence conflict')
        ranges.append(dict(rva=rva, size=size, unit=section['object'],
                           name=section['name'], kind='data-section'))
    return ranges


def generate(image, synth: dict, data_rows: list[dict], output=OUTPUT, metadata=METADATA, *, data_sections=()):
    ranges = []
    for rva, (name, unit, size) in synth['names_map'].items():
        if unit and size:
            ranges.append(dict(rva=rva, size=size, unit=unit, name=name, kind='function'))
    for row in data_rows:
        ranges.append(dict(rva=row['rva'], size=row['size'], unit=row['object'],
                           name=row['name'], kind='data'))
    ranges.extend(section_ranges(data_rows, data_sections))
    ranges.sort(key=lambda row: (row['rva'], row['size'], row['unit']))
    selected = select(image.absolute_sites, [(r['rva'], r['rva'] + r['size']) for r in ranges])
    digest = hashlib.sha256(image.data).hexdigest()
    body = '\n'.join([f'# image-sha256: {digest}',
                      '# Generated subset: all admitted fields within delink_scope.json ranges.',
                      'site_rva\tkind', *(f'0x{site:08x}\tdir32' for site in selected)]) + '\n'
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(body)
    master = RETAIL / 'absolute_relocations.tsv'
    metadata.write_text(json.dumps(dict(
        image_sha256=digest, master_manifest=str(master),
        master_sha256=hashlib.sha256(master.read_bytes()).hexdigest(),
        generated_manifest_sha256=hashlib.sha256(body.encode()).hexdigest(),
        full_inventory_fields=len(image.absolute_sites), selected_fields=len(selected),
        policy='all fields intersecting selected code/data; no target-identity filter',
        ranges=ranges), indent=2) + '\n')
    # Exercise exactly the same hash/structural gate as the external tool input.
    if image.reviewed_sites(output) != selected:
        raise ValueError('generated delink scope failed round-trip validation')
    return output
