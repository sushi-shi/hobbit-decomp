"""VC6 static functions own EH metadata; ordinary local labels do not."""
import unittest
from types import SimpleNamespace as NS
from hobbit.compare.canonicalize import _eh_funclet_owners


class StaticEhOwner(unittest.TestCase):
    def fixture(self, storage=3, typ=0x20):
        return NS(
            data=b'\x68\0\0\0\0\xc3',
            symbols={0: NS(index=0, section=1, storage_class=storage, typ=typ,
                           name='?Handler@@YAXXZ', value=0),
                     1: NS(index=1, section=2, storage_class=6, typ=0,
                           name='$L1', value=0)},
            sections=[NS(raw_offset=0, raw_size=5, characteristics=0x20000020),
                      NS(raw_offset=5, raw_size=1, characteristics=0x20000020)],
            relocations=[NS(section=1, site=1, typ=6, symbol_index=1)])

    def test_static_function_has_same_eh_ownership_as_external_function(self):
        for storage in (2, 3):
            self.assertEqual(_eh_funclet_owners(self.fixture(storage=storage)),
                             {1: '__ehreg$?Handler@@YAXXZ'})

    def test_untyped_static_label_cannot_supply_function_identity(self):
        self.assertFalse(_eh_funclet_owners(self.fixture(typ=0)))
