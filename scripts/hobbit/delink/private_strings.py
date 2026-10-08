"""VC6 unpooled $SG literals, preserving each physical target instance.

Port of HoMM1 Buka d3c9297135800ee0d198b292f2424464d20b6f5c's
sg_literal_rows/_paired_votes contract. Address comes from matching code
references; terminated bytes and storage are then independently rechecked.
Names retain the $SG compiler-private family and no source datum is invented.
"""
from __future__ import annotations
import re
from hobbit.delink import coffx
from hobbit.delink.literal_refs import referenced_addresses

_MEMBER = re.compile(r'^\$SG[0-9]+$')
_STORAGE = {'data-initialized':'data', 'rdata':'rdata'}


def rows(model, image, base_dir):
    result, withheld = [], []
    for unit, obj in coffx.objects(base_dir):
        pool = {}
        for section in obj.section_table:
            storage = {'.data':'data','.rdata':'rdata'}.get(section['name'])
            if storage is None:
                continue
            members = obj.section_members(section['index'])
            starts = sorted({offset for offset, _name, _storage in members})
            raw = obj.section_payload(section['index'])
            for offset, name, _scl in members:
                if not _MEMBER.fullmatch(name):
                    continue
                end = next((at for at in starts if at > offset), section['size'])
                value = raw[offset:end]
                nul = value.find(b'\0')
                if nul < 0:
                    withheld.append((0,name,'unterminated candidate private string'))
                    continue
                pool[name] = (storage, value[:nul + 1])
        if not pool:
            continue
        addresses = referenced_addresses(model,image,unit,obj,pool, admitted_strings=True)
        for member,(storage,payload) in sorted(pool.items()):
            targets = addresses[member]
            if len(targets) != 1:
                withheld.append((0,member,'private string needs one corroborated physical address'))
                continue
            rva = next(iter(targets))
            start,end = image.classify_storage(rva),image.classify_storage(rva + len(payload) - 1)
            if _STORAGE.get(start) != storage or start != end or image.pe.read(rva,len(payload)) != payload:
                withheld.append((rva,member,'private string bytes/storage contradict candidate'))
                continue
            result.append(dict(name=f'$SG{rva}',member=member,object=f'{unit}.c',rva=rva,
                               size=len(payload),storage=storage,provenance='retail-reference-private-string'))
    return result,withheld
