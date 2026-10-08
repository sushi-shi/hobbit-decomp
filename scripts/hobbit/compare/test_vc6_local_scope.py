"""VC6 local-static scope masking preserves source identity and references."""

import struct
import unittest

from hobbit.compare import canonicalize
from hobbit.compare.canonicalize import CoffObject, _rewrite_names
from hobbit.compare.test_data_boundaries import obj

ORIGINAL = '?SpaceArray@?4??AddEndLine@text_out@@QAEXXZ@4QBDB'
CANONICAL = '?SpaceArray@?1??AddEndLine@text_out@@QAEXXZ@4QBDB'


def fixture(name=ORIGINAL, second='_other', **kwargs):
    return _rewrite_names(CoffObject(obj(**kwargs)), {0: name, 1: second})


def reference(raw):
    coff = CoffObject(raw)
    relocation = coff.relocations[0]
    symbol = coff.symbols[relocation.symbol_index]
    section = coff.sections[relocation.section - 1]
    addend = struct.unpack_from('<I', raw, section.raw_offset + relocation.site)[0]
    return symbol.name, relocation.typ, addend


class LocalScopeControls(unittest.TestCase):
    def test_ordinal_joins_same_source_identity(self):
        for name in (ORIGINAL, ORIGINAL.replace('@?4??', '@?BC@??')):
            with self.subTest(name=name):
                result = canonicalize.canonicalize_coff(fixture(name))
                self.assertEqual(reference(result.data), (CANONICAL, 6, 0x40))
                before, after = CoffObject(fixture(name)), CoffObject(result.data)
                self.assertEqual(before.sections, after.sections)
                for left, right in zip(before.sections, after.sections):
                    self.assertEqual(before.section_bytes(left), after.section_bytes(right))
                self.assertEqual(canonicalize.canonicalize_coff(result.data).data, result.data)

    def test_name_and_owner_remain_distinct(self):
        for name in (ORIGINAL.replace('SpaceArray', 'Other'),
                     ORIGINAL.replace('AddEndLine', 'OtherFn'),
                     ORIGINAL.replace('text_out', 'other_class')):
            with self.subTest(name=name):
                result = canonicalize.canonicalize_coff(fixture(name)).data
                self.assertNotEqual(reference(result)[0], CANONICAL)

    def test_wrong_reference_is_not_retargeted(self):
        for target, addend in ((0, 1), (0, 0x41), (0, 0xFFFFFFFF), (1, 0)):
            with self.subTest(target=target, addend=addend):
                raw = fixture(target=target, addend=addend)
                result = canonicalize.canonicalize_coff(raw).data
                self.assertEqual(reference(raw)[1:], reference(result)[1:])
                if target == 1:
                    self.assertEqual(reference(result)[0], '_other')

    def test_undefined_reference_keeps_identity(self):
        result = canonicalize.canonicalize_coff(fixture(undefined=True)).data
        self.assertEqual(reference(result)[0], ORIGINAL)

    def test_colliding_scope_definitions_fail(self):
        for second in (CANONICAL, ORIGINAL.replace('@?4??', '@?5??')):
            with self.subTest(second=second):
                with self.assertRaisesRegex(ValueError, 'identity collision'):
                    canonicalize.canonicalize_coff(fixture(second=second))

    def test_function_and_label_are_not_local_data(self):
        for typ, storage, section in ((0x20, 3, 2), (0, 6, 2), (0, 3, 1)):
            with self.subTest(typ=typ, storage=storage, section=section):
                raw = bytearray(fixture())
                symbol = CoffObject(raw).symbols[0]
                struct.pack_into('<hH', raw, symbol.offset + 12, section, typ)
                raw[symbol.offset + 16] = storage
                result = canonicalize.canonicalize_coff(bytes(raw)).data
                self.assertEqual(reference(result)[0], ORIGINAL)

    def test_wrong_data_payload_is_not_changed(self):
        raw = bytearray(fixture())
        section = CoffObject(raw).sections[1]
        raw[section.raw_offset + 3] ^= 0xFF
        result = canonicalize.canonicalize_coff(bytes(raw)).data
        after = CoffObject(result)
        self.assertEqual(bytes(raw[section.raw_offset:section.raw_offset + section.raw_size]),
                         after.section_bytes(after.sections[1]))


if __name__ == '__main__':
    unittest.main()
