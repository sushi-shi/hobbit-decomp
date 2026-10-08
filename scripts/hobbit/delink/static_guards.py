"""VC6 emitted byte guards for source-owned function-local static objects.

Uses Gruntz's local-static ownership contract (7d4bd55b99e32f084834d991badf7609889481f4)
and HoMM1 Buka's paired-reference address proof (d3c9297135800ee0d198b292f2424464d20b6f5c).
The actual compiler symbol encodes a byte and the enclosing function. No guard
variable or volatile ordinal is added to source or the admitted retail facts.
"""
import re
from hobbit.core.msvc_names import mask
from hobbit.delink import coffx
from hobbit.delink.literal_refs import referenced_addresses

_GUARD = re.compile(r'^\?\$S[0-9]+(@\?(?:[0-9]|[A-P]+@)\?\?.+)@4EA$')


def rows(model, image, base_dir):
    result, withheld = [], []
    for unit, obj in coffx.objects(base_dir):
        guards = set()
        for section in obj.section_table:
            if section['name'] != '.bss' or not section['characteristics'] & 0x80:
                continue
            for _offset, name, storage in obj.section_members(section['index']):
                match = _GUARD.fullmatch(name)
                if not match or storage != 3:
                    continue
                scope = mask(match[1])
                owner = '?' + scope.split('??', 1)[1]
                source_function = any(b.channel == 'src' and b.unit == unit and mask(b.name) == owner
                                      for b in model.functions if b.name)
                source_object = any(b.channel == 'src' and b.unit == unit and scope + '@4' in mask(b.name)
                                    for b in model.data if b.name)
                if source_function and source_object:
                    guards.add(name)
        if not guards:
            continue
        addresses = referenced_addresses(model, image, unit, obj, guards)
        for member in sorted(guards):
            targets = addresses[member]
            if len(targets) != 1:
                withheld.append((0, member, 'local-static byte guard needs one corroborated address'))
                continue
            rva = next(iter(targets))
            if (image.classify_storage(rva) not in ('data-loader-zero-tail', 'data-unprovable-tail')
                    or image.pe.read(rva, 1) != b'\0'):
                withheld.append((rva, member, 'local-static byte guard lacks compatible zero storage'))
                continue
            result.append(dict(name=member, member=member, object=f'{unit}.c', rva=rva,
                               size=1, storage='bss', provenance='candidate-local-static-byte-guard'))
    return result, withheld
