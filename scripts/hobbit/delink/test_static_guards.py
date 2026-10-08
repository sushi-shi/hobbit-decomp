import struct
import unittest
from types import SimpleNamespace as NS
from unittest.mock import patch
from hobbit.delink.static_guards import rows

OWNER='?Use@@YAXXZ'
GUARD='?$S9@?1??Use@@YAXXZ@4EA'
OBJECT='?value@?1??Use@@YAXXZ@4VThing@@A'


class Candidate:
    section_table=[dict(index=1,name='.text',size=6,characteristics=0x20000020),
                   dict(index=2,name='.bss',size=1,characteristics=0x80)]
    def section_members(self,section):return [(0,GUARD,3)] if section==2 else [(0,OWNER,3)]
    def defined_symbols(self,section):return []
    def section_payload(self,section):return b'\xa0'+bytes(4)+b'\xc3' if section==1 else b''
    def typed_relocations(self,section):return {1:(GUARD,6)}


class LocalGuardControls(unittest.TestCase):
    def run_case(self, *, owner=True, datum=True, reviewed=True, storage='data-loader-zero-tail', byte=b'\0'):
        model=NS(functions=[NS(name=OWNER,rva=0x1000,size=6,unit='fixture',channel='src')] if owner else [],
                 data=[NS(name=OBJECT,rva=0x3100,size=4,unit='fixture',channel='src')] if datum else [])
        image=NS(image_base=0x400000,pe=NS(read=lambda rva,size:
                    b'\xa0'+struct.pack('<I',0x403000)+b'\xc3' if rva==0x1000 else byte),
                 relocs_in=lambda lo,hi:[0x1001] if reviewed else [],classify_storage=lambda rva:storage)
        with patch('hobbit.delink.static_guards.coffx.objects',return_value=[('fixture',Candidate())]):
            return rows(model,image,None)

    def test_compiler_guard_requires_its_source_function_object_and_exact_reference(self):
        result,_=self.run_case()
        self.assertEqual([(r['name'],r['rva'],r['size']) for r in result],[(GUARD,0x3000,1)])
        for kwargs in (dict(owner=False),dict(datum=False),dict(reviewed=False),
                       dict(storage='data-initialized'),dict(byte=b'x')):
            with self.subTest(kwargs=kwargs):self.assertEqual(self.run_case(**kwargs)[0],[])


if __name__=='__main__':unittest.main()
