"""Coverage and semantic grouping controls for the integer census."""

import tempfile
import unittest
import contextlib
import io
from pathlib import Path
from unittest import mock

from hobbit.verify import constants


class ConstantInventoryTests(unittest.TestCase):
    def scan(self, source):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            path = root / "src/Probe.cpp"
            path.parent.mkdir()
            path.write_text(source)
            entry = {"directory": str(root), "file": "src/Probe.cpp",
                     "arguments": ["clang-cl", "/c", "src/Probe.cpp", "/TP"]}
            sites, errors = constants.scan_entries([entry], repo=root)
            self.assertEqual(errors, [])
            return constants.complete_source_coverage(sites, repo=root)

    def test_lexical_backstop_includes_inactive_and_unused_macro_literals(self):
        sites = self.scan('#define UNUSED 17\n#if 0\nint hidden=23;\n#endif\n'
                          'int x=010; const char* text="42"; // 99\n'
                          'float f=1.25e+3f; int number42=8;\n')
        self.assertEqual(sorted(s.value for s in sites), [0, 8, 8, 17, 23])
        self.assertEqual(next(s for s in sites if s.spelling == "010").value, 8)
        self.assertEqual(next(s for s in sites if s.value == 23).coverage, "lexical")

    def test_unused_header_and_address_annotation_are_accounted(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            header = root / "include/Unused.h"
            header.parent.mkdir()
            header.write_text('RVA(0x1000, 4)\nstruct Unused { int data[37]; };\n')
            sites = constants.complete_source_coverage([], repo=root)
            self.assertEqual([s.value for s in sites], [4096, 4, 37])
            self.assertEqual([s.review_group for s in sites[:2]], ["source-label"] * 2)
            self.assertEqual(sites[2].review_group, "unparsed-source")

    def test_continued_string_contents_are_not_integer_literals(self):
        sites = self.scan('const char* text="a\\\n42"; int value=7;\n')
        self.assertEqual([s.value for s in sites], [7])
        self.assertEqual(sites[0].line, 2)

    def test_resource_script_ids_and_coordinates_are_covered_separately(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            path = root / "src/Probe.rc"
            path.parent.mkdir()
            path.write_text('101 DIALOG 0, 0, 120, 80\nCAPTION "version 42"\n')
            sites = constants.complete_source_coverage([], repo=root)
            self.assertEqual([s.value for s in sites], [101, 0, 0, 120, 80])
            self.assertEqual({s.review_group for s in sites}, {"resource-script"})

    def test_same_field_assignment_compare_and_switch_share_identity(self):
        sites = self.scan('struct State { int mode; int other; };\n'
                          'void F(State& s) { s.mode=7; if(s.mode==8) {} '
                          'switch(s.mode) {case 9: break;} s.other=10; }\n')
        by_value = {s.value: s.context_key for s in sites}
        self.assertTrue(by_value[7])
        self.assertEqual(by_value[7], by_value[8])
        self.assertEqual(by_value[8], by_value[9])
        self.assertNotEqual(by_value[9], by_value[10])

    def test_callee_owners_overloads_and_composed_arguments_stay_distinct(self):
        sites = self.scan('struct A { void Set(int); }; struct B { void Set(int); };\n'
                          'void Set(int); void Set(double);\n'
                          'void F(A& a,B& b,int n) { a.Set(11); b.Set(12); '
                          'Set(13); Set(double(14)); Set(n+15); Set(16); }\n')
        keys = {s.value: s.context_key for s in sites}
        self.assertEqual(keys[13], keys[16])
        self.assertEqual(len({keys[v] for v in [11, 12, 13, 14, 15]}), 5)

    def test_zeros_keep_semantic_destinations_without_automatic_disposition(self):
        sites = self.scan('struct Record {int version;}; '
                          'void F(Record& r) { r.version=1; if(r.version==0){} }')
        self.assertEqual(sites[0].context_key, sites[1].context_key)
        self.assertIn("version", sites[0].context_label)
        with tempfile.TemporaryDirectory() as temp:
            report = Path(temp) / "groups.tsv"
            constants.write_groups(report, sites)
            self.assertIn("pending-review", report.read_text())

    def test_source_edit_during_scan_cannot_produce_successful_census(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            path = root / "src/Probe.cpp"
            path.parent.mkdir()
            path.write_text("int n=7;")
            entry = {"directory": str(root), "file": "src/Probe.cpp",
                     "arguments": ["clang-cl", "/c", "src/Probe.cpp", "/TP"]}
            scan_entry = constants._scan_entry

            def edit(payload):
                result = scan_entry(payload)
                path.write_text("int n=999;")
                return result

            with mock.patch.object(constants, "_scan_entry", side_effect=edit):
                _, errors = constants.scan_entries([entry], repo=root)
            self.assertTrue(any("source changed" in error for error in errors))

    def test_failed_parse_removes_previous_successful_reports(self):
        with tempfile.TemporaryDirectory() as temp:
            report = Path(temp) / "bare_constants.tsv"
            groups = report.with_name("constant_contexts.tsv")
            report.write_text("old success")
            groups.write_text("old success")
            with mock.patch.object(constants, "REPORT", report), \
                 mock.patch.object(constants, "scan", return_value=([], ["parse failed"])), \
                 contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(constants.main([]), 2)
            self.assertFalse(report.exists())
            self.assertFalse(groups.exists())


if __name__ == "__main__":
    unittest.main()
