"""Actual VC6 value spellings do not replace address and payload evidence."""
import struct
import unittest
from types import SimpleNamespace as NS
from unittest.mock import patch
from hobbit.delink.real_constants import rows


class RealConstants(unittest.TestCase):
    def run_case(self, *, name='__real@3f800000', payload=b'\0\0\x80?', storage='rdata', wrong_field=False, wrong_code=False, table_boundary=None, other_difference=False):
        class Candidate:
            section_table=[dict(index=1,name='.text',size=7,characteristics=0x20000020),
                           dict(index=2,name='.rdata',size=4,characteristics=0x40001040)]
            def section_members(self,s):
                return ([(0,name,2)] if s==2 else
                        ([(table_boundary,'$Lswitch',3)] if table_boundary is not None else []))
            def section_payload(self,s):
                return (b'\xd9\x05\0\0\0\0\xc3' +
                        (b'\x12\x34\x56\x78' if table_boundary is not None else b'')) if s==1 else payload
            def defined_symbols(self,s):return [(0,'_Load')]
            def typed_relocations(self,s):return {2:(name,6)}
        code=(b'\xdd' if wrong_code else b'\xd9')+b'\x05'+struct.pack('<I',0x403000)+(b'\x90' if other_difference else b'\xc3')
        image=NS(image_base=0x400000,pe=NS(read=lambda r,n:code if r==0x1000 else b'\0\0\x80?'),
                 relocs_in=lambda lo,hi:[] if wrong_field else [0x1002],classify_storage=lambda r:storage)
        model=NS(functions=[NS(rva=0x1000,size=7,name='_Load',unit='fixture',channel='src')],data=[])
        with patch('hobbit.delink.real_constants.coffx.objects',return_value=[('fixture',Candidate())]):
            return rows(model,image,None)

    def test_reference_and_exact_payload_prove_constant(self):
        result,withheld=self.run_case()
        self.assertEqual([(r['name'],r['rva'],r['size'])for r in result],[('__real@3f800000',0x3000,4)])
        self.assertFalse(withheld)

    def test_wrong_name_bytes_storage_instruction_or_unreviewed_operand_refused(self):
        for kwargs in (dict(name='__real@40000000'),dict(payload=bytes(4)),dict(storage='data-initialized'),dict(wrong_field=True),dict(wrong_code=True)):
            with self.subTest(kwargs=kwargs):
                result,withheld=self.run_case(**kwargs)
                self.assertFalse(result)
                self.assertTrue(withheld)

    def test_appended_switch_table_does_not_extend_instruction_extent(self):
        result, withheld = self.run_case(table_boundary=7)
        self.assertEqual(len(result), 1)
        self.assertFalse(withheld)

    def test_label_inside_claim_cannot_hide_overlapping_extent(self):
        result, withheld = self.run_case(table_boundary=6)
        self.assertFalse(result)
        self.assertTrue(withheld)

    def test_decoded_fp_operand_can_be_proved_while_code_still_differs(self):
        with patch('hobbit.delink.data_manifest._fp_read_widths', return_value=[{2: 4}, {2: 4}]) as decode:
            result, withheld = self.run_case(other_difference=True)
        self.assertEqual(len(result), 1)
        self.assertFalse(withheld)
        candidate, retail = decode.call_args.args[0]
        self.assertNotEqual(candidate[-1], retail[-1])

    def test_fp_operand_requires_decoded_boundary_and_matching_width(self):
        for decoded in ([{}, {2: 4}], [{2: 4}, {}], [{2: 8}, {2: 8}]):
            with self.subTest(decoded=decoded), patch('hobbit.delink.data_manifest._fp_read_widths', return_value=decoded):
                result, withheld = self.run_case(other_difference=True)
            self.assertFalse(result)
            self.assertTrue(withheld)
