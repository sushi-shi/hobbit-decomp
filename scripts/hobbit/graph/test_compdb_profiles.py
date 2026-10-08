"""Parse configuration must reflect each era compiler profile."""
import unittest

from hobbit.graph.compdb import unit_flags


class ProfileFlags(unittest.TestCase):
    def test_preserves_defines_and_includes_and_translates_exceptions(self):
        self.assertEqual(unit_flags(["/nologo", "/c", "/O2", "/MT", "/GX", "/GR",
                                     "/DTARGET_PC", "/I", "include/custom", "/Zp4"]),
                         ["/MT", "/EHsc", "/GR", "/DTARGET_PC", "/I", "include/custom", "/Zp4"])

    def test_missing_rtti_switch_means_off(self):
        self.assertEqual(unit_flags(["/O2", "/MT"]), ["/MT", "/EHs-c-", "/GR-"])

    def test_missing_include_argument_rejected(self):
        with self.assertRaises(ValueError):
            unit_flags(["/I"])

class EraUtilityMirror(unittest.TestCase):
    def test_only_redundant_defaults_removed_without_mutating_donor(self):
        import tempfile
        from pathlib import Path
        from hobbit.graph.compdb import normalize_era_utility
        raw=b'body-before\r\n'+b''.join(b'template<class _E, class _Tr = char_traits<_E> >\r\n\tclass '+n+b' { genuine-body; };\r\n' for n in (b'istreambuf_iterator',b'ostreambuf_iterator'))
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);original=root/'UTILITY';original.write_bytes(raw);mirror=root/'mirror';mirror.mkdir();(mirror/'utility').symlink_to(original)
            normalize_era_utility(mirror);once=(mirror/'utility').read_bytes()
            self.assertEqual(original.read_bytes(),raw)
            self.assertEqual(once,raw.replace(b'_Tr = char_traits<_E> >',b'_Tr>'))
            normalize_era_utility(mirror);self.assertEqual((mirror/'utility').read_bytes(),once)
