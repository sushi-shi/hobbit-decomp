"""VC6 SEH scope tables, using the local donor's EH-table/reference contract.

Like data_manifest.ehfuncinfo_rows (Gruntz/HoMM1), metadata is emitted by
its source compiler and associated with a proved function. Unlike C++ EH,
SEH records contain an enclosing level and two function-local code labels.
Only a single observed 12-byte scope record is currently supported.
"""
from __future__ import annotations
import struct
from hobbit.core.msvc_names import mask
from hobbit.delink import coffx
from hobbit.delink.literal_refs import referenced_addresses


def rows(model, image, base_dir):
    result, withheld = [], []
    for unit, obj in coffx.objects(base_dir):
        extents = {mask(b.name): b for b in model.functions
                   if b.channel and b.unit == unit and b.name}
        for sec in obj.section_table:
            if sec['name'] != '.rdata' or sec.get('comdat') != 5 or sec['size'] != 12:
                continue
            members = obj.section_members(sec['index'])
            if len(members) != 1:
                continue
            off, member, storage = members[0]
            if off or storage != 3 or not member.startswith('$T'):
                continue
            owner_section = sec.get('assoc')
            owners = [(offset, extents[mask(name)]) for offset, name in obj.defined_symbols(owner_section)
                      if mask(name) in extents]
            if len(owners) != 1:
                continue
            start, owner = owners[0]
            payload = obj.section_payload(sec['index'])
            refs = obj.typed_relocations(sec['index'])
            if payload[:4] != b'\xff' * 4 or set(refs) != {4, 8}:
                continue
            labels = {obj.sym_name(i): (value, section) for i, value, section in obj.iter_symbols()}
            expected = bytearray(payload)
            valid = True
            for site, (name, kind) in refs.items():
                label, section = labels.get(name, (-1, -1))
                if kind != 6 or section != owner_section or not start <= label < start + owner.size:
                    valid = False
                    break
                addend = struct.unpack_from('<i', payload, site)[0]
                value = image.image_base + owner.rva + label - start + addend
                struct.pack_into('<I', expected, site, value)
            if not valid:
                continue
            targets = referenced_addresses(model, image, unit, obj, {member})[member]
            if len(targets) != 1:
                withheld.append((0, member, 'SEH table lacks exact owner reference'))
                continue
            rva = next(iter(targets))
            if (image.classify_storage(rva) != 'rdata' or
                    image.classify_storage(rva + 11) != 'rdata' or
                    image.pe.read(rva, 12) != expected or
                    set(image.relocs_in(rva, rva + 12)) != {rva + 4, rva + 8}):
                withheld.append((rva, member, 'SEH scope bytes or reviewed code pointers disagree'))
                continue
            result.append(dict(name=f'$T{rva}', member=member, object=f'{unit}.c',
                               rva=rva, size=12, storage='rdata',
                               provenance='candidate-COFF-SEH-scope'))
    return result, withheld
