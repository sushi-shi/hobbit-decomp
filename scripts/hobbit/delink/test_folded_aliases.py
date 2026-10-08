"""Source-owned folded procedures; all PE input is an isolated fixture."""

from io import StringIO
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from hobbit.delink import pdb_synth
from hobbit.model import Binding, Model
from hobbit.retail_labels import Claim


class FoldedAliasControls(unittest.TestCase):
    def setUp(self):
        self.alias = Claim(0x1100, "??0Color@@QAE@XZ", "func", "src", 3,
                           "color", {})
        self.binding = Binding(0x1100, 3, "", "text", "??0Vector@@QAE@XZ",
                               "vector", "src", (self.alias,))
        self.funcs = [(self.binding.rva, self.binding.size, self.binding.name)]

    def records(self, alias=None):
        binding = self.binding if alias is None else self.binding._replace(
            aliases=(alias,))
        return pdb_synth.folded_alias_records(Model([binding], [], []), self.funcs)

    def emit(self, aliases=()):
        text = dict(va=0x1000, vsize=0x1000, rsize=0x1000)
        rdata = dict(va=0x3000, vsize=0x1000, rsize=0x1000)
        pe = SimpleNamespace(sections=[text, rdata], section=lambda name: text)
        output = StringIO()
        with patch.object(pdb_synth, "retail", return_value=SimpleNamespace(pe=pe)):
            pdb_synth.emit_yaml(
                self.funcs, [(0x3020, "_constant")], [], [],
                {0x1100: (self.binding.name, "vector", 3)}, output, aliases)
        return output.getvalue()

    def test_source_identity_retains_extent_and_owner(self):
        self.assertEqual(self.records(), [(0x1100, 3, self.alias.name, "color")])

    def test_compiler_supplied_source_identity_is_retained(self):
        self.assertEqual(self.records(self.alias._replace(channel="src_compgen")),
                         self.records())

    def test_unequal_or_missing_extent_rejected(self):
        for size in (None, 0, 2, 4):
            with self.subTest(size=size), self.assertRaises(ValueError):
                self.records(self.alias._replace(size=size))

    def test_different_address_rejected(self):
        with self.assertRaises(ValueError):
            self.records(self.alias._replace(rva=0x1101))

    def test_missing_source_owner_rejected(self):
        with self.assertRaises(ValueError):
            self.records(self.alias._replace(unit=""))

    def test_nonprocedure_and_non_source_channels_are_excluded(self):
        for change in (dict(kind="data"), dict(channel="src_dyninit"),
                       dict(channel="functions_static_libs"),
                       dict(channel="src_data_compgen")):
            with self.subTest(change=change):
                self.assertEqual(self.records(self.alias._replace(**change)), [])

    def test_no_identity_is_invented_for_missing_alias(self):
        model = Model([self.binding._replace(aliases=())], [], [])
        self.assertEqual(pdb_synth.folded_alias_records(model, self.funcs), [])
        self.assertEqual(self.records(self.alias._replace(name="")), [])

    def test_unselected_or_shortened_procedure_is_not_introduced(self):
        model = Model([self.binding], [], [])
        for funcs in ([], [(0x1100, 2, self.binding.name)],
                      [(0x1100, 3, "unrelated")]):
            with self.subTest(funcs=funcs):
                self.assertEqual(pdb_synth.folded_alias_records(model, funcs), [])

    def test_duplicate_claims_do_not_duplicate_procedures(self):
        model = Model([self.binding._replace(aliases=(self.alias, self.alias))], [], [])
        self.assertEqual(pdb_synth.folded_alias_records(model, self.funcs),
                         self.records())

    def test_single_owner_fixture_keeps_real_section_offsets(self):
        yaml = self.emit()
        self.assertEqual(yaml.count("    - Module:"), 1)
        self.assertNotIn("folded_alias_", yaml)
        self.assertEqual(yaml.count("- Kind:            S_GPROC32"), 1)
        self.assertIn("DisplayName:     '??0Vector@@QAE@XZ'", yaml)
        self.assertEqual(yaml.count("CodeSize:        3\n"), 2)
        self.assertIn("RelocOffset:     256\n", yaml)
        self.assertIn("Offset:          256\n", yaml)
        self.assertIn("FileName:        'c:\\proj\\vector.c'", yaml)
        self.assertIn("Offset:          32\n              Segment:         2\n"
                      "              DisplayName:     '_constant'", yaml)

    def test_folded_procedures_keep_separate_full_extent_source_ownership(self):
        yaml = self.emit(self.records())
        modules = yaml.split("    - Module:")[1:]
        self.assertEqual(len(modules), 2)
        first, second = modules
        self.assertIn("DisplayName:     '??0Vector@@QAE@XZ'", first)
        self.assertIn("FileName:        'c:\\proj\\vector.c'", first)
        self.assertNotIn("??0Color", first)
        self.assertIn("DisplayName:     '??0Color@@QAE@XZ'", second)
        self.assertIn("FileName:        'c:\\proj\\color.c'", second)
        self.assertNotIn("??0Vector", second)
        self.assertNotIn("S_LDATA32", second)
        for module in modules:
            self.assertEqual(module.count("CodeSize:        3\n"), 2)
            self.assertIn("RelocOffset:     256\n", module)
            self.assertIn("Offset:          256\n", module)
        self.assertEqual(yaml.split("StringTable:\n")[1],
                         "  - 'c:\\proj\\vector.c'\n  - 'c:\\proj\\color.c'\n")


if __name__ == "__main__":
    unittest.main()
