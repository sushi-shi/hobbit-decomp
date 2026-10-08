"""Exact-site numeric reviews and wrapped instructions in the Gruntz audit port."""
import hashlib
from pathlib import Path
from tempfile import TemporaryDirectory
from types import SimpleNamespace as NS
import unittest
from unittest import mock

from hobbit.core import tsv
from hobbit.verify import access_map


class DataAccessFieldTests(unittest.TestCase):
    def image(self, code):
        return NS(base=0x400000, reloc={},
                  pe=NS(data=code, sections=[dict(va=0x400000, vsize=4, rsize=4)]),
                  read=lambda rva, size: code[rva-0x1000:rva-0x1000+size])

    def test_wrapped_mov_is_one_complete_instruction(self):
        listing = ("  248c83:\tc7 05 f4 cf 7c 00 28 \tmov DWORD PTR ds:0x7ccff4,0x74c628\n"
                   "  248c8a:\tc6 74 00 \n")
        with mock.patch('hobbit.tool.objdump.disassemble', return_value=listing):
            dec = access_map._decode(bytes.fromhex('c705f4cf7c0028c67400'), 0x248c83)
        self.assertEqual(dec.starts, [0x248c83])
        self.assertEqual(dec.length(0), 10)
        self.assertEqual(dec.at(0x248c89), 0)

    def test_numeric_review_requires_hash_and_exact_site_bytes_and_opcode(self):
        img = self.image(bytes.fromhex('a900008000'))
        header = ['site_rva', 'raw_hex', 'instruction_rva', 'instruction_hex', 'evidence']
        row = dict(zip(header, ['0x1001', '00008000', '0x1000', 'a900008000', 'reviewed bit mask']))
        banner = ['# image-sha256: ' + hashlib.sha256(img.pe.data).hexdigest()]
        with TemporaryDirectory() as td:
            path = Path(td)/'numeric.tsv'
            tsv.write(path, banner, header, [row])
            self.assertEqual(access_map.reviewed_nonpointer_operands(img, path),
                             {0x1001: (0x1000, img.pe.data)})
            for field, wrong in [('site_rva', '0x1002'), ('raw_hex', '01008000'),
                                 ('instruction_rva', '0x1001'), ('instruction_hex', 'a801008000')]:
                with self.subTest(field=field):
                    tsv.write(path, banner, header, [dict(row, **{field: wrong})])
                    with self.assertRaises(ValueError):
                        access_map.reviewed_nonpointer_operands(img, path)
            tsv.write(path, ['# image-sha256: '+'0'*64], header, [row])
            with self.assertRaises(ValueError):
                access_map.reviewed_nonpointer_operands(img, path)
            tsv.write(path, banner, header, [row])
            img.reloc = {0x1001: 0x400000}
            with self.assertRaises(ValueError):
                access_map.reviewed_nonpointer_operands(img, path)

    def test_only_reviewed_immediate_is_numeric_not_memory_displacement(self):
        for opcode, text, expected in [('a9', 'test eax,0x800000', []),
                                       ('a1', 'mov eax,ds:0x800000', None)]:
            with self.subTest(opcode=opcode):
                code = bytes.fromhex(opcode+'00008000');img = self.image(code)
                model = NS(functions=[NS(channel='src', rva=0x1000, size=5)])
                dec = access_map.Decode([0x1000], [text], set(), [5])
                with mock.patch.object(access_map, '_decode', return_value=dec), \
                     mock.patch.object(access_map, 'reviewed_nonpointer_operands',
                                       return_value={0x1001:(0x1000,code)}):
                    count,gaps=access_map.scoped_reference_gaps(img, model)
                self.assertEqual(count, 1)
                if expected is None:
                    self.assertEqual(len(gaps), 1)
                else:
                    self.assertEqual(gaps, expected)

    def test_missing_review_keeps_numeric_operand_unresolved(self):
        img=self.image(bytes.fromhex('a900008000'))
        model=NS(functions=[NS(channel='src',rva=0x1000,size=5)])
        dec=access_map.Decode([0x1000],['test eax,0x800000'],set(),[5])
        with mock.patch.object(access_map,'_decode',return_value=dec), \
             mock.patch.object(access_map,'reviewed_nonpointer_operands',return_value={}):
            self.assertEqual(len(access_map.scoped_reference_gaps(img,model)[1]),1)

if __name__ == '__main__':
    unittest.main()
