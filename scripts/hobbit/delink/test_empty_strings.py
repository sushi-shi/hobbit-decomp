"""Empty pooled constants require both an actual reference and zero storage."""
import struct
import unittest
from types import SimpleNamespace as NS
from unittest.mock import patch
from hobbit.delink.empty_strings import rows


class Candidate:
    section_table = [dict(index=1,name='.text',size=6,characteristics=0x20000020),
                     dict(index=2,name='.bss',size=1,characteristics=0x80),
                     dict(index=3,name='.bss',size=1,characteristics=0x80)]
    def iter_symbols(self):return [(0,0,1),(1,0,2),(2,0,3)]
    def sym_name(self,index):return ['_Use','??_C@a','??_C@b'][index]
    def section_payload(self,section):return b'\x68\0\0\0\0\xc3' if section==1 else b''
    def typed_relocations(self,section):return {1:('??_C@a',6)}
    def defined_symbols(self,section):return [(0,'_Use')]


class EmptyStringControls(unittest.TestCase):
    def run_case(self, *, site=True, payload=b'\0', storage='data-loader-zero-tail', opcode=0x68):
        code=bytes([opcode])+struct.pack('<I',0x403000)+b'\xc3'
        image=NS(image_base=0x400000,
                 relocs_in=lambda lo,hi:[0x1001] if site else [],
                 pe=NS(read=lambda rva,size:code if rva==0x1000 else payload),
                 classify_storage=lambda rva:storage)
        model=NS(functions=[NS(name='_Use',rva=0x1000,size=6,unit='fixture',channel='src')],data=[])
        with patch('hobbit.delink.empty_strings.coffx.objects',return_value=[('fixture',Candidate())]):
            return rows(model,image,None)

    def test_exact_reference_provides_only_the_referenced_empty_literal(self):
        result,withheld=self.run_case()
        self.assertEqual([(r['name'],r['rva'],r['size'],r['storage']) for r in result],
                         [('??_C@a',0x3000,1,'bss')])
        self.assertEqual([name for _,name,_ in withheld],['??_C@b'])

    def test_zero_memory_without_reference_or_matching_instruction_is_insufficient(self):
        for kwargs in (dict(site=False),dict(opcode=0xb8),dict(payload=b'x'),dict(payload=None),
                       dict(storage='data-initialized')):
            with self.subTest(kwargs=kwargs):self.assertEqual(self.run_case(**kwargs)[0],[])

    def test_candidate_bss_disambiguates_file_alignment_slack(self):
        result,_=self.run_case(storage='data-unprovable-tail')
        self.assertEqual(result[0]['storage'],'bss')

    def test_unpooled_bss_member_requires_one_byte_extent_and_a_reference(self):
        class Unpooled(Candidate):
            section_table=[Candidate.section_table[0],dict(index=2,name='.bss',size=5,characteristics=0x80)]
            def iter_symbols(self):return [(0,0,1),(1,4,2)]
            def sym_name(self,index):return ['_Use','$SG1'][index]
            def typed_relocations(self,section):return {1:('$SG1',6)}
            def section_members(self,section):return [(0,'_Other',3),(4,'$SG1',3)] if section==2 else []
        image=NS(image_base=0x400000,relocs_in=lambda lo,hi:[0x1001],
                 pe=NS(read=lambda rva,size:b'\x68\0\x30\x40\0\xc3' if rva==0x1000 else b'\0'),
                 classify_storage=lambda rva:'data-loader-zero-tail')
        model=NS(functions=[NS(name='_Use',rva=0x1000,size=6,unit='fixture',channel='src')],data=[])
        with patch('hobbit.delink.empty_strings.coffx.objects',return_value=[('fixture',Unpooled())]):
            result,_=rows(model,image,None)
        self.assertEqual([(r['name'],r['member'],r['size']) for r in result],[('$SG12288','$SG1',1)])


if __name__=='__main__':unittest.main()
