"""VC6 __real@ COMDATs, proved from exact code references and literal bytes.

Adapts HoMM1 Buka d3c9297135800ee0d198b292f2424464d20b6f5c
 audit/placements.py's FP_POOL reference/byte proof to VC6 value spellings,
using reviewed operand positions, known referents, and exact x87 reads.
Unrelated instruction differences remain in the code comparison. No content scans.
"""
from __future__ import annotations
import re
from hobbit.delink import coffx
from hobbit.delink.literal_refs import referenced_addresses

NAME = re.compile(r'^__real@([0-9a-f]{8}|[0-9a-f]{16})$')


def rows(model, image, base_dir):
    result, withheld = [], []
    for unit, obj in coffx.objects(base_dir):
        pool = {}
        for section in obj.section_table:
            if section['name'] != '.rdata' or not section['characteristics'] & 0x1000:
                continue
            members = obj.section_members(section['index'])
            if len(members) != 1:
                continue
            offset, name, storage = members[0]
            match = NAME.fullmatch(name)
            if not match or offset != 0 or storage != 2:
                continue
            payload = bytes.fromhex(match[1])[::-1]
            if section['size'] != len(payload) or obj.section_payload(section['index']) != payload:
                withheld.append((0,name,'real constant name does not prove candidate bytes/extent'))
                continue
            pool[name] = payload
        addresses = referenced_addresses(model,image,unit,obj,pool,
                    x87_widths={name: len(payload) for name, payload in pool.items()}) if pool else {}
        for name,payload in sorted(pool.items()):
            targets = addresses[name]
            if len(targets) != 1:
                withheld.append((0,name,'real constant needs one corroborated physical address'))
                continue
            rva = next(iter(targets))
            if (image.classify_storage(rva) != 'rdata' or
                    image.classify_storage(rva+len(payload)-1) != 'rdata' or
                    image.pe.read(rva,len(payload)) != payload):
                withheld.append((rva,name,'real constant bytes/storage contradict candidate'))
                continue
            result.append(dict(name=name,member=name,object=f'{unit}.c',rva=rva,
                               size=len(payload),storage='rdata',provenance='candidate-COFF-real'))
    return result,withheld
