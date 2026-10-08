import struct
import unittest
from types import SimpleNamespace as NS
from unittest.mock import patch
from hobbit.delink.seh_tables import rows


class SehTables(unittest.TestCase):
    def run_case(self, *, wrong_pointer=False, unreviewed=False, outside=False, wrong_owner=False):
        class Candidate:
            section_table = [dict(index=1, name='.text', size=20, characteristics=0x20000020),
                             dict(index=2, name='.rdata', size=12, characteristics=0x40001040, comdat=5, assoc=1)]
            def section_members(self, sec):
                return [(0, '$T12', 3)] if sec == 2 else [(0, '_NameThread', 2), (6, '$Lfilter', 3), (10, '$Lhandler', 3)]
            def section_payload(self, sec):
                return b'\x68' + bytes(4) + b'\xc3' + b'\x90' * 14 if sec == 1 else b'\xff' * 4 + bytes(8)
            def typed_relocations(self, sec):
                return {1: ('$T12', 6)} if sec == 1 else {4: ('$Lfilter', 6), 8: ('$Lhandler', 6)}
            def defined_symbols(self, sec):
                return [(0, '_NameThread')] if sec == 1 else []
            def iter_symbols(self):
                return [(0, 6, 1), (1, 20 if outside else 10, 1)]
            def sym_name(self, i):
                return ['$Lfilter', '$Lhandler'][i]
        code = b'\x68' + struct.pack('<I', 0x403000) + b'\xc3' + b'\x90' * 14
        if wrong_owner:
            code = b'\x69' + code[1:]
        table = struct.pack('<III', 0xffffffff, 0x401007 if wrong_pointer else 0x401006, 0x40100a)
        image = NS(image_base=0x400000,
                   pe=NS(read=lambda r, n: code if r == 0x1000 else table),
                   classify_storage=lambda r: 'rdata',
                   relocs_in=lambda lo, hi: [0x1001] if lo == 0x1000 else ([] if unreviewed else [0x3004, 0x3008]))
        model = NS(functions=[NS(rva=0x1000, size=20, name='_NameThread', unit='fixture', channel='src')], data=[])
        with patch('hobbit.delink.seh_tables.coffx.objects', return_value=[('fixture', Candidate())]):
            return rows(model, image, None)

    def test_exact_owner_and_two_local_code_pointers(self):
        result, withheld = self.run_case()
        self.assertEqual([(r['rva'], r['size'], r['member']) for r in result], [(0x3000, 12, '$T12')])
        self.assertFalse(withheld)

    def test_wrong_pointer_unreviewed_field_outside_label_or_owner_rejected(self):
        for case in ('wrong_pointer', 'unreviewed', 'outside', 'wrong_owner'):
            with self.subTest(case=case):
                result, _ = self.run_case(**{case: True})
                self.assertFalse(result)


if __name__ == '__main__':
    unittest.main()
