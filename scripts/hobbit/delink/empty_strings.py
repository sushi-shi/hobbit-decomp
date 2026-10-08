"""Address VC6's one-byte empty pooled strings in loader-zero storage.

Derived from HoMM1 Buka d3c9297135800ee0d198b292f2424464d20b6f5c
`delink/data_manifest.py` empty_bss proof. Hobbit emits ??_C@ COMDATs under
/O2 and ordinary $SG members under its explicit unpooled profile. A zero byte
never supplies an address: an exact reference from a claimed owner must
corroborate the candidate symbol first.
"""
from __future__ import annotations
import re
from hobbit.delink import coffx
from hobbit.delink.literal_refs import referenced_addresses


def rows(model, image, base_dir):
    result, withheld = [], []
    for unit, obj in coffx.objects(base_dir):
        literals = set()
        for index, offset, section in obj.iter_symbols():
            name = obj.sym_name(index)
            pooled = name.startswith('??_C@')
            private = bool(re.fullmatch(r'\$SG[0-9]+', name))
            if section <= 0 or not (pooled or private):
                continue
            sec = obj.section_table[section - 1]
            end = sec['size']
            if private:
                end = min((value for value, _name, _storage in obj.section_members(section)
                           if value > offset), default=end)
            if (sec['name'] == '.bss' and end - offset == 1
                    and sec['characteristics'] & 0x80 and not obj.section_payload(section)):
                literals.add(name)
        if not literals:
            continue
        addresses = referenced_addresses(model, image, unit, obj, literals)
        for name in sorted(literals):
            targets = addresses[name]
            if len(targets) != 1:
                withheld.append((0, name, 'empty pooled string needs one corroborated address'))
                continue
            rva = next(iter(targets))
            storage = image.classify_storage(rva)
            if storage not in ('data-loader-zero-tail', 'data-unprovable-tail') or image.pe.read(rva, 1) != b'\0':
                withheld.append((rva, name, 'empty pooled string lacks compatible zero storage'))
                continue
            private = name.startswith('$SG')
            result.append(dict(name=f'$SG{rva}' if private else name, member=name,
                               object=f'{unit}.c', rva=rva, size=1, storage='bss',
                               provenance='retail-reference-private-string' if private else 'candidate-COFF-string'))
    return result, withheld
