"""Provide pinned initializer identities from their owned datum and COFF graph.

Adapts Gruntz static_dtors.py's candidate helper/retail reference pairing.
A volatile $E name is never a source claim. Both sides must prove the same
owner reference, exact callee identities, addends, and non-address bytes.
"""
from __future__ import annotations
import re
import struct
from collections import defaultdict
from hobbit.core.msvc_names import mask
from hobbit.delink.coffx import Obj

_HELPER = re.compile(r'^_?\$E[0-9]+$')


def provision(model, names_map, base_dir, image):
    pins = [b for b in model.functions if b.channel == 'src_dyninit' and not b.name]
    if not pins:
        return {}
    by_unit = defaultdict(list)
    for pin in pins:
        by_unit[pin.unit].append(pin)
    result = {}
    for unit, pending in by_unit.items():
        path = base_dir / f'{unit}.obj'
        if not path.is_file():
            continue
        obj = Obj(path)
        helpers = []
        for index, start, section in obj.iter_symbols():
            name = obj.sym_name(index)
            if section <= 0 or not _HELPER.fullmatch(name):
                continue
            if not obj.section_table[section - 1]['characteristics'] & 0x20000000:
                continue
            payload = obj.section_payload(section)
            following = [offset for offset, _name, _scl in obj.section_members(section)
                         if offset > start]
            end = min(following, default=len(payload))
            relocs = {site - start: pair for site, pair in obj.typed_relocations(section).items()
                      if start <= site < end}
            helpers.append((name, payload[start:end], relocs))
        known = {mask(b.name): b.rva for b in model.data if b.channel and b.name and b.unit == unit}
        known.update({mask(name): rva for rva, (name, _unit, _size) in names_map.items()})
        used = set()
        owned_helpers = {}
        while pending:
            progress = False
            for pin in list(pending):
                owners = {a.name for a in pin.aliases if a.channel == 'src_dyninit'}
                owned_data = {b.rva for b in model.data if b.channel and b.unit == unit
                              and any(mask(b.name) in (owner, '_' + owner) for owner in owners)}
                permitted = owned_data | {rva for rva, owner in owned_helpers.items() if owner == owners}
                retail = image.raw_read(pin.rva, pin.size)
                if retail is None:
                    continue
                matches = []
                for name, payload, relocs in helpers:
                    if name in used or len(payload) < pin.size or any(b not in (0x90, 0xcc) for b in payload[pin.size:]):
                        continue
                    left, right = bytearray(payload[:pin.size]), bytearray(retail)
                    owner_seen = False
                    valid = True
                    for offset, (target_name, kind) in relocs.items():
                        target = known.get(mask(target_name))
                        if offset + 4 > pin.size or target is None or kind not in (6, 20):
                            valid = False
                            break
                        addend = struct.unpack_from('<i', payload, offset)[0]
                        value = struct.unpack_from('<I', retail, offset)[0]
                        if kind == 6:
                            actual = value - image.image_base
                            if pin.rva + offset not in image.absolute_sites:
                                valid = False
                                break
                        else:
                            actual = pin.rva + offset + 4 + struct.unpack('<i', retail[offset:offset + 4])[0]
                        if actual != target + addend:
                            valid = False
                            break
                        owner_seen |= target in permitted
                        left[offset:offset + 4] = right[offset:offset + 4] = bytes(4)
                    if valid and owner_seen and left == right:
                        # Every admitted absolute field must have a counterpart;
                        # otherwise identical numeric addresses could hide debt.
                        abs_offsets = {site - pin.rva for site in image.relocs_in(pin.rva, pin.rva + pin.size)}
                        if abs_offsets == {off for off, (_target, typ) in relocs.items() if typ == 6}:
                            matches.append(name)
                if len(matches) != 1:
                    continue
                name = matches[0]
                result[pin.rva] = (name, unit, pin.size)
                known[mask(name)] = pin.rva
                owned_helpers[pin.rva] = owners
                used.add(name)
                pending.remove(pin)
                progress = True
            if not progress:
                break
    return result
