"""Compiler-observed names; VC6 12.00.9044 array_names.cpp fixture."""
import unittest
from hobbit.core.msvc_names import data, mask, discriminate

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


class InitializerRecordNamesTests(unittest.TestCase):
    def test_distinct_vc6_initializer_slots_keep_distinct_names(self):
        # AudioMgr's actual .CRT$XCU slots 0,4,8,12,16,20 independently
        # relocate to _$E34,37,40,43,46,49. These are not one source static.
        names = [f"_$S{ordinal}" for ordinal in (35, 38, 41, 44, 47, 50)]
        self.assertEqual([mask(name) for name in names], names)
        self.assertEqual(len({mask(name) for name in names}), len(names))

    def test_named_static_suffix_and_scope_rules_still_normalize(self):
        self.assertEqual(mask("_?s_x@?BA@??F@@QAEHXZ@4HA$S35536"),
                         "_?s_x@?1??F@@QAEHXZ@4HA$S")
        self.assertEqual(mask("_?$S47@?1??G@@QAEHXZ@4EA$S20267"),
                         "_?$S@?1??G@@QAEHXZ@4EA$S")
        self.assertEqual(mask("_owner_$S35"), "_owner_$S")

    def test_rva_discriminators_retain_existing_contract(self):
        self.assertEqual(mask("$S2277272"), "$S2277272")
        self.assertEqual(mask(discriminate("_s_x$S", 0x244970)), "_s_x$S")

if __name__ == '__main__':
    unittest.main()
