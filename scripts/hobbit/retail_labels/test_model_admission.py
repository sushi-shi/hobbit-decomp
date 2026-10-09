"""Gruntz census joins, extent bounds, and genuine compiler-data kinds."""
import unittest
from unittest.mock import patch

from hobbit import model
from hobbit.retail_labels import Claim


class ModelAdmission(unittest.TestCase):
    def resolve(self, kind, size):
        claims = [Claim(0x1000, "?probe@@YAXXZ", "func", "src", size, "probe", {})]
        with patch.object(model.censuses, "functions", return_value=[dict(rva=0x1000, size=16, kind=kind)]), \
             patch.object(model.censuses, "data", return_value=[]), \
             patch.object(model.providers, "all_claims", return_value=[]), \
             patch.object(model.src_claims, "all_claims", return_value=claims), \
             patch.object(model, "_materialized", side_effect=lambda cs, problems: cs), \
             patch.object(model, "_band_owner_fn", return_value=lambda rva: None):
            return model.resolve()

    def test_unknown_interval_is_not_admitted_function(self):
        result = self.resolve("unknown", 4)
        self.assertTrue(any("binds kind='unknown'" in v for v in result.violations))

    def test_claim_may_not_cross_admitted_start(self):
        result = self.resolve("", 17)
        self.assertTrue(any("crosses the next admitted start" in v for v in result.violations))

    def test_exact_extent_within_valid_function(self):
        result = self.resolve("", 4)
        self.assertEqual(result.violations, [])
        self.assertEqual(result.functions[0].size, 4)


class DataAdmission(unittest.TestCase):
    def resolve(self, kind="", size=4, unit="probe"):
        claims = [Claim(0x2000, "_datum", "data", "src", size, unit, {})]
        with patch.object(model.censuses, "functions", return_value=[]), \
             patch.object(model.censuses, "data", return_value=[dict(rva=0x2000, size=16, kind=kind, region="data")]), \
             patch.object(model.providers, "all_claims", return_value=[]), \
             patch.object(model.src_claims, "all_claims", return_value=claims), \
             patch.object(model, "_materialized", side_effect=lambda cs, problems: cs), \
             patch.object(model, "_band_owner_fn", return_value=lambda rva: None):
            return model.resolve()

    def test_label_only_claim_uses_census_extent(self):
        result = self.resolve(size=None)
        self.assertEqual(result.violations, [])
        self.assertEqual(result.data[0].size, 16)

    def test_claim_cannot_cross_next_data_start(self):
        self.assertTrue(any("crosses the next admitted start" in v
                            for v in self.resolve(size=17).violations))

    def test_unknown_data_kind_remains_visible_after_source_join(self):
        result = self.resolve(kind="unknown")
        self.assertEqual(result.violations, [])
        self.assertEqual(result.data[0].name, "_datum")
        self.assertEqual(result.data[0].kind, "unknown")

    def test_source_claim_does_not_name_bookkeeping_storage(self):
        for kind in ("pad", "ehtable"):
            with self.subTest(kind=kind):
                self.assertTrue(any("binds bookkeeping" in v
                                    for v in self.resolve(kind=kind).violations))

    def test_reviewed_data_with_positive_extent_passes(self):
        result = self.resolve()
        self.assertEqual(result.violations, [])
        self.assertEqual(result.data[0].size, 4)


class FragmentOwnership(unittest.TestCase):
    def test_unconfigured_fragment_is_never_read(self):
        from pathlib import Path
        import tempfile
        from hobbit.retail_labels import fragments
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            (root / "old_unit.tsv").write_text("invalid stale content must not be parsed")
            with patch.object(fragments, "FRAGMENTS", root), \
                 patch("hobbit.manifest.units", return_value=[]):
                self.assertEqual(fragments.all_claims(), [])


class ContributionOwnership(unittest.TestCase):
    def test_absent_optional_table_has_no_owner(self):
        with patch.object(model.censuses, "link_order_bands", side_effect=FileNotFoundError):
            self.assertIsNone(model._band_owner_fn()(0x1000))

    def test_malformed_optional_table_falls_back_without_owner(self):
        from pathlib import Path
        import tempfile
        reader = model.censuses.link_order_bands
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "link_order.tsv"
            path.write_text("seq\tunit\tstart\tend\tclass\tmodule\tn\tevidence\tnotes\n"
                            "0\tprobe\t0x1000\tnot-a-hex-address\tcmdline\tgame\t1\tfixture\t\n")
            with patch.object(model.censuses, "link_order_bands", side_effect=lambda: reader(path)):
                self.assertIsNone(model._band_owner_fn()(0x1000))

    def test_unreadable_optional_table_falls_back_without_owner(self):
        with patch.object(model.censuses, "link_order_bands", side_effect=PermissionError):
            self.assertIsNone(model._band_owner_fn()(0x1000))

    def test_valid_bands_preserve_half_open_boundaries(self):
        with patch.object(model.censuses, "link_order_bands", return_value=[(0x1000, 0x1010, "probe")]):
            owner = model._band_owner_fn()
        self.assertIsNone(owner(0xFFF))
        self.assertEqual(owner(0x1000), "probe")
        self.assertEqual(owner(0x100F), "probe")
        self.assertIsNone(owner(0x1010))


class CompilerDataKinds(unittest.TestCase):
    def kind(self, name, channel="src_data_compgen"):
        return model._data_expected_kind(
            Claim(0x2000, name, "data", channel, 4, "probe", {}))

    def test_valid_named_float_and_double_pools(self):
        for name in ("__real@447a0000", "__real@3ff0000000000000", "$T123"):
            with self.subTest(name=name):
                self.assertEqual(self.kind(name), "fppool")

    def test_invalid_named_pools_remain_strings(self):
        for name in ("__real@447a000", "__real@447a00000",
                     "__real@447A0000", "__real@447a000g",
                     "__real@447a0000suffix", "??_C@_03@string", "_datum"):
            with self.subTest(name=name):
                self.assertEqual(self.kind(name), "string")

    def test_named_pool_does_not_change_other_data_channels(self):
        self.assertIsNone(self.kind("__real@447a0000", "src"))
        self.assertIsNone(self.kind("__real@447a0000", "data_compgen"))
