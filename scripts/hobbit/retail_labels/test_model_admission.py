"""Unknown retail intervals must never become function identities by a claim."""
import unittest
from unittest.mock import patch

from hobbit import model
from hobbit.retail_labels import Claim


class ModelAdmission(unittest.TestCase):
    def resolve(self, kind, size):
        claims = [Claim(0x1000, "?probe@@YAXXZ", "func", "src", size, "probe", {})]
        with patch.object(model, "manifest_units", return_value=[{"unit": "probe"}]), \
             patch.object(model.src_claims, "stale_fragments", return_value=[]), \
             patch.object(model.censuses, "functions", return_value=[dict(rva=0x1000, size=16, kind=kind)]), \
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
        with patch.object(model, "manifest_units", return_value=[{"unit": "probe"}]), \
             patch.object(model.src_claims, "stale_fragments", return_value=[]), \
             patch.object(model.censuses, "functions", return_value=[]), \
             patch.object(model.censuses, "data", return_value=[dict(rva=0x2000, size=16, kind=kind, region="data")]), \
             patch.object(model.providers, "all_claims", return_value=[]), \
             patch.object(model.src_claims, "all_claims", return_value=claims), \
             patch.object(model, "_materialized", side_effect=lambda cs, problems: cs), \
             patch.object(model, "_band_owner_fn", return_value=lambda rva: None):
            return model.resolve()

    def test_nonpositive_extent_is_not_bound(self):
        for size in (-1, 0):
            result = self.resolve(size=size)
            self.assertTrue(any("nonpositive extent" in v for v in result.violations))
            self.assertEqual(result.claimed(), [])

    def test_missing_extent_is_not_bound(self):
        self.assertTrue(any("no proved extent" in v for v in self.resolve(size=None).violations))

    def test_unconfigured_owner_is_not_bound(self):
        result = self.resolve(unit="removed_unit")
        self.assertTrue(any("not configured" in v for v in result.violations))
        self.assertEqual(result.claimed(), [])

    def test_unknown_data_interval_rejected(self):
        self.assertTrue(any("kind='unknown'" in v for v in self.resolve(kind="unknown").violations))

    def test_reviewed_data_with_positive_extent_passes(self):
        result = self.resolve()
        self.assertEqual(result.violations, [])
        self.assertEqual(result.data[0].size, 4)


class FragmentOwnership(unittest.TestCase):
    def test_stale_fragment_is_reported_and_never_read(self):
        from pathlib import Path
        import tempfile
        from hobbit.retail_labels import fragments
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            (root / "old_unit.tsv").write_text("invalid stale content must not be parsed")
            with patch.object(fragments, "FRAGMENTS", root), \
                 patch("hobbit.manifest.units", return_value=[]):
                self.assertEqual(fragments.all_claims(), [])
                self.assertEqual(fragments.stale_fragments(), ["old_unit"])


class ContributionOwnership(unittest.TestCase):
    def test_absent_optional_table_has_no_owner(self):
        with patch.object(model.censuses, "link_order_bands", side_effect=FileNotFoundError):
            self.assertIsNone(model._band_owner_fn()(0x1000))

    def test_malformed_existing_table_propagates(self):
        from pathlib import Path
        import tempfile
        reader = model.censuses.link_order_bands
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "link_order.tsv"
            path.write_text("seq\tunit\tstart\tend\tclass\tmodule\tn\tevidence\tnotes\n"
                            "0\tprobe\t0x1000\tnot-a-hex-address\tcmdline\tgame\t1\tfixture\t\n")
            with patch.object(model.censuses, "link_order_bands", side_effect=lambda: reader(path)):
                with self.assertRaises(ValueError):
                    model._band_owner_fn()

    def test_existing_unreadable_table_propagates(self):
        with patch.object(model.censuses, "link_order_bands", side_effect=PermissionError):
            with self.assertRaises(PermissionError):
                model._band_owner_fn()

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
