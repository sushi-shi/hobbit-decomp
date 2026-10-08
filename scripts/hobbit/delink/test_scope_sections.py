"""Complete affine section scope, including the vtable COL prefix."""
import unittest

from hobbit.delink.scope import section_ranges, select


class ScopeSections(unittest.TestCase):
    def fixture(self):
        # COFF index differs from serialized ordinal; the symbol starts after COL.
        section = dict(object='unit.c', index=37, ordinal=2, rva=0x3000,
                       size=12, storage='rdata', name='.rdata',
                       provenance='candidate-COFF-section')
        row = dict(object='unit.c', section_ordinal=2, section_offset=4,
                   rva=0x3004, size=8, storage='rdata')
        return row, section

    def test_complete_col_prefix_uses_manifest_ordinal(self):
        row, section = self.fixture()
        ranges = section_ranges([row], [section])
        self.assertEqual([(r['rva'], r['size']) for r in ranges], [(0x3000, 12)])
        self.assertEqual(select([0x3000, 0x3004, 0x3008],
                                [(r['rva'], r['rva'] + r['size']) for r in ranges]),
                         [0x3000, 0x3004, 0x3008])

    def test_unrelated_nonaffine_section_is_excluded(self):
        row, section = self.fixture()
        unrelated = dict(section, rva=None, object='other.c', ordinal=99,
                         provenance='candidate-COFF-section-nonaffine')
        self.assertEqual(section_ranges([row], [section, unrelated]),
                         section_ranges([row], [section]))

    def test_invalid_owner_storage_base_extent_and_identity(self):
        row, section = self.fixture()
        for mutation in (dict(object='other.c'), dict(storage='bss'),
                         dict(rva=0x3004), dict(size=8), dict(size=0),
                         dict(rva=-1), dict(ordinal=0),
                         dict(provenance='unproved')):
            with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                section_ranges([row], [dict(section, **mutation)])
        with self.assertRaises(ValueError):
            section_ranges([row], [section, section])

    def test_invalid_member_bounds_and_affine_position(self):
        row, section = self.fixture()
        for mutation in (dict(section_offset=None), dict(section_offset=-1),
                         dict(section_offset=8), dict(size=0), dict(size=9),
                         dict(rva=0x3008), dict(section_ordinal=37)):
            with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                section_ranges([dict(row, **mutation)], [section])

    def test_partial_absolute_field_fails_closed(self):
        with self.assertRaises(ValueError):
            select([0x3000], [(0x3001, 0x300c)])
        with self.assertRaises(ValueError):
            select([0x3008], [(0x3000, 0x300a)])


if __name__ == '__main__':
    unittest.main()
