"""Hobbit target portability and fail-closed scope controls (Gruntz adapters)."""
import contextlib
import io
from pathlib import Path
import tempfile
from types import SimpleNamespace as NS
import unittest
from unittest import mock

from hobbit.verify import access_map, alloc_size, tiers, universe


class HobbitScopeControls(unittest.TestCase):
    def test_unknown_text_is_not_a_proven_function_denominator(self):
        model = NS(functions=[NS(channel="", kind="unknown", size=100),
                              NS(channel="src", kind="", size=7)])
        result = universe.engine_universe(model)
        self.assertEqual((result["real_fn"], result["real_code"], result["unknown_code"]), (1, 7, 100))

    def test_score_numerator_excludes_compiler_helper_spans(self):
        from hobbit.verify import readme, scores
        model = NS(functions=[NS(rva=0x1000, channel="src", kind="", size=7),
                              NS(rva=0x1100, channel="src_dyninit", kind="", size=5)])
        doc = {"units": [{"name": "probe", "functions": [
            dict(name="ordinary", size=7, fuzzy_match_percent=100),
            dict(name="helper", size=5, fuzzy_match_percent=100)],
            "measures": dict(total_functions=2, matched_functions=2,
                             total_code=12, matched_code=12, fuzzy_match_percent=100)}]}
        filtered, cur, sizes, other = readme.target_rollup(
            doc, model, {("probe", "ordinary"): 0x1000, ("probe", "helper"): 0x1100})
        self.assertEqual(cur, {("probe", "ordinary"): 100})
        self.assertEqual(sizes, {("probe", "ordinary"): 7})
        self.assertEqual(other, (1, 5, 1))
        self.assertEqual(scores.unit_measures(filtered)["probe"]["total_functions"], 1)
        self.assertEqual(doc["units"][0]["measures"]["total_functions"], 2)


    def test_missing_object_cannot_make_allocation_scope_empty(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            (root / "probe.cpp").write_text("int f() {return 1;}")
            with mock.patch.object(alloc_size, "REPO", root), mock.patch.object(alloc_size, "BUILD", root), \
                 mock.patch("hobbit.manifest.units", return_value=[dict(unit="probe", source="probe.cpp")]):
                with self.assertRaises(FileNotFoundError):
                    alloc_size.empty_allocation_scope()

    def test_not_applicable_and_deferred_do_not_print_ok(self):
        registry = {"fixture": [("empty", lambda: tiers.NotApplicable("0 verified allocation sites")),
                                ("missing", None)]}
        with mock.patch.object(tiers, "TIERS", registry), contextlib.redirect_stdout(io.StringIO()) as output:
            self.assertEqual(tiers.run(["fixture"]), 1)
        self.assertIn("NOT APPLICABLE", output.getvalue())
        self.assertNotIn(": OK", output.getvalue())

    def test_no_import_section_is_a_valid_region_set(self):
        pe = NS(data_regions=lambda: {"rdata": (0x2000, 0x2100), "data": (0x3000, 0x3100), "bss": (0x3100, 0x3100)})
        self.assertEqual(access_map.data_ranges(pe), [(".rdata", 0x2000, 0x2100), (".data", 0x3000, 0x3100)])


if __name__ == "__main__":
    unittest.main()
