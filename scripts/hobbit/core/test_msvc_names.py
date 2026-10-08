"""Compiler-observed names; VC6 12.00.9044 array_names.cpp fixture."""
import unittest
from hobbit.core.msvc_names import data

class ArrayNamesTests(unittest.TestCase):
    def test_const_class_member_arrays_keep_their_qualifier(self):
        for name in ('?private_array@arrays@@0QBUitem@@B',
                     '?public_const_array@arrays@@2QBUitem@@B',
                     '?m_FormatInfo@xbitmap@@2QBUformat_info@1@B'):
            self.assertEqual(data(name, internal=False, decorated=True), name)

    def test_existing_nonmember_array_rewrite_is_preserved(self):
        self.assertEqual(data('?values@@3QBHB', internal=False, decorated=True),
                         '?values@@3PBHB')

    def test_mutable_class_member_arrays_are_unchanged(self):
        for name in ('?public_array@arrays@@2PAHA', '?protected_array@arrays@@1PAHA'):
            self.assertEqual(data(name, internal=False, decorated=True), name)

if __name__ == '__main__':
    unittest.main()
