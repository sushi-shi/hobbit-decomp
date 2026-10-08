import struct
import unittest
from types import SimpleNamespace as NS
from hobbit.compare.canonicalize import _rewrite_jump_table_relocations


class Vc6JumpTable(unittest.TestCase):
    def run_case(self, storage=3, name='$L123', other_owner=False):
        raw = bytearray(96)
        struct.pack_into('<I', raw, 42, 3)
        struct.pack_into('<I', raw, 24, 1)
        owner = NS(index=0, section=1, value=0, typ=0x20, storage_class=2, name='_Function')
        target = NS(index=1, section=1, value=10, typ=0, storage_class=storage, name=name)
        symbols = {0: owner, 1: target}
        if other_owner:
            symbols[2] = NS(index=2, section=1, value=8, typ=0x20, storage_class=2, name='_Other')
        obj = NS(data=bytes(raw), symbols=symbols,
                 sections=[NS(characteristics=0x20000020, raw_size=20, raw_offset=40)],
                 relocations=[NS(typ=6, section=1, symbol_index=1, site=2, offset=20)])
        return bytes(raw), _rewrite_jump_table_relocations(obj, bytes(raw))

    def test_vc6_static_and_older_label_preserve_resolved_address(self):
        for storage in (3, 6):
            before, (after, rewrites) = self.run_case(storage=storage)
            self.assertEqual(len(rewrites), 1)
            self.assertEqual(struct.unpack_from('<I', after, 42)[0], 13)
            self.assertEqual(struct.unpack_from('<I', after, 24)[0], 0)
            self.assertEqual(rewrites[0].resolved_offset, 13)

    def test_arbitrary_static_or_cross_function_label_is_not_rewritten(self):
        for options in (dict(name='_datum'), dict(other_owner=True)):
            before, (after, rewrites) = self.run_case(**options)
            self.assertEqual(before, after)
            self.assertFalse(rewrites)
