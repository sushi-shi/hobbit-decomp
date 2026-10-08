"""Regression coverage for complete GNU objdump instruction encodings."""
import unittest
from unittest.mock import Mock, patch
from hobbit.sema.disasm import decode

class WrappedInstructionTests(unittest.TestCase):
    def test_contiguous_wrapped_bytes_belong_to_previous_instruction(self):
        raw = bytes.fromhex('c744243c00000000c3')
        listing = ('  25068b:\tc7 44 24 3c 00 00 00 \tmov DWORD PTR [esp+0x3c],0x0\n'
                   '  250692:\t00 \n'
                   '  250693:\tc3                   \tret\n')
        with patch('hobbit.sema.disasm.retail', return_value=Mock(read=Mock(return_value=raw))), \
             patch('hobbit.sema.disasm.objdump.disassemble', return_value=listing):
            result = decode(0x25068b, len(raw))
        self.assertEqual([i.raw for i in result], [raw[:8], raw[8:]])
        self.assertEqual(result[0].end, result[1].rva)
        self.assertEqual(result[0].mnemonic, 'mov')

    def test_noncontiguous_bytes_are_not_appended(self):
        listing = ('  1000:\tc3 \tret\n'
                   '  1008:\t00 \n')
        with patch('hobbit.sema.disasm.retail', return_value=Mock(read=Mock(return_value=b'\xc3'))), \
             patch('hobbit.sema.disasm.objdump.disassemble', return_value=listing):
            result = decode(0x1000, 1)
        self.assertEqual(result[0].raw, b'\xc3')

if __name__ == '__main__':
    unittest.main()
