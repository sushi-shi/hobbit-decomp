"""Named integer coverage and cross-domain review controls."""

import csv
import tempfile
import unittest
from pathlib import Path

from hobbit.verify import enum_reuse as reuse


class NamedConstantCoverage(unittest.TestCase):
    def scan(self, source):
        with tempfile.TemporaryDirectory() as directory:
            repo = Path(directory)
            path = repo / "src/Probe.cpp"
            path.parent.mkdir()
            path.write_text(source)
            blocks = reuse.scan_blocks(repo=repo)
            raw, contexts, errors = reuse.scan_entries([{
                "directory": str(repo), "file": "src/Probe.cpp",
                "arguments": ["clang-cl", "/c", "src/Probe.cpp", "/TP"],
            }], repo=repo)
            constants, missing, unmapped = reuse._join(raw, contexts, blocks)
            self.assertEqual(errors, [])
            return constants, blocks, missing, unmapped, list(raw.values())

    def test_named_aliases_target_width_and_runtime_exclusion(self):
        rows, blocks, missing, unmapped, observations = self.scan(
            "enum Domain { TEN = 10 };\n"
            "#define LIMIT (TEN * 2)\n"
            "#define HIGH_BIT (1u << 31)\n"
            "#define SEMICOLON ';'\n"
            "const unsigned int cap = LIMIT;\n"
            "const int negative = -TEN;\n"
            "int read(); void f() { const int runtime = read(); }\n"
        )
        self.assertEqual(missing, [])
        self.assertEqual(unmapped, [])
        self.assertEqual({row.name: row.value for row in rows},
                         {"TEN": 10, "LIMIT": 20, "HIGH_BIT": 2147483648, "SEMICOLON": 59,
                          "cap": 20, "negative": -10})
        runtime = next(row for row in observations if row.name == "runtime")
        self.assertEqual(runtime.status, "excluded")
        self.assertIn("runtime", runtime.reason)

    def test_multiline_macro_and_nonintegral_declarations(self):
        rows, _, missing, unmapped, observations = self.scan(
            "#define LIMIT (2 + \\\n3)\n"
            '#define TEXT "hello"\n'
            "const float fraction = 1.5f;\n"
            "const int first = 2, second = first + 3;\n"
        )
        self.assertEqual(missing, [])
        self.assertEqual(unmapped, [])
        self.assertEqual({row.name: row.value for row in rows},
                         {"LIMIT": 5, "first": 2, "second": 5})
        self.assertEqual(next(row.status for row in observations if row.name == "fraction"), "excluded")

    def test_unresolved_macro_is_visible_not_zero(self):
        rows, _, missing, unmapped, observations = self.scan(
            "#define LIMIT MISSING_EXTERNAL_NAME\n"
        )
        self.assertEqual(rows, [])
        self.assertEqual(len(missing), 1)
        self.assertEqual(len(unmapped), 1)
        self.assertEqual(observations[0].status, "unresolved")
        self.assertIsNone(observations[0].value)

    def test_existing_ledger_cannot_silently_claim_new_named_domain(self):
        rows, _, _, _, _ = self.scan("const int limit = 9;\n")
        with tempfile.TemporaryDirectory() as directory:
            ledger = Path(directory) / "review.tsv"
            reuse._write_tsv(ledger, reuse.LEDGER_FIELDS, [])
            self.assertTrue(any("no starting-ledger provenance" in finding
                                for finding in reuse.check_ledger(ledger, rows)))

    def test_unused_header_postfix_const_comma_and_parenthesized_macros(self):
        with tempfile.TemporaryDirectory() as directory:
            repo = Path(directory)
            header = repo / "include/Unused.h"
            header.parent.mkdir()
            header.write_text(
                "typedef unsigned long Count;\n"
                "Count const FIRST = 1, SECOND = 2;\n"
                "#define OBJECT (3 + 4)\n"
                "#define FUNCTION(x) ((x) + 4)\n"
                "#if 0\nconst Count INACTIVE = 8;\n#endif\n"
            )
            blocks = reuse.scan_blocks(repo=repo)
            self.assertEqual({member.name for block in blocks for member in block.members},
                             {"FIRST", "SECOND", "OBJECT", "INACTIVE"})
            _, missing, _ = reuse._join({}, {}, blocks)
            self.assertEqual(len(missing), 4)

    def test_pair_ranking_uses_same_value_and_destination(self):
        rows, _, missing, unmapped, _ = self.scan(
            "enum Left { A=5, B=8 }; enum Right { C=5, D=9 };\n"
            "enum NumericOnly { E=5, F=8 };\n"
            "void consume(int); void f(){ consume(A); consume(C); }\n"
        )
        self.assertEqual(missing + unmapped, [])
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "pairs.tsv"
            reuse.write_pair_report(report, rows)
            with report.open() as stream:
                pairs = list(csv.DictReader(stream, dialect="excel-tab"))
            self.assertTrue(pairs[0]["left"].endswith(":Left"))
            self.assertTrue(pairs[0]["right"].endswith(":Right"))
            self.assertIn("call:", pairs[0]["direct_shared_contexts"])

    def test_macro_expansion_keeps_argument_and_array_index_domains_separate(self):
        rows, _, missing, unmapped, _ = self.scan(
            "enum Left { LEFT=1 }; enum Right { RIGHT=1 };\n"
            "void consume(int,int); int table[4];\n"
            "#define SEND() consume(LEFT, table[RIGHT])\n"
            "void f(){ SEND(); }\n")
        self.assertEqual(missing + unmapped, [])
        by_name = {row.name: row for row in rows}
        left = by_name["LEFT"].use_contexts
        right = by_name["RIGHT"].use_contexts
        self.assertEqual(len(left), 1)
        self.assertTrue(left[0].endswith(":argument:1"), left)
        self.assertEqual(len(right), 1)
        self.assertTrue(right[0].startswith("index:"), right)
        self.assertFalse(set(left) & set(right))

    def test_macro_expansion_distinguishes_sibling_arguments(self):
        rows, _, missing, unmapped, _ = self.scan(
            "enum Left { LEFT=1 }; enum Right { RIGHT=1 }; void consume(int,int);\n"
            "#define SEND() consume(LEFT, RIGHT)\nvoid f(){ SEND(); }\n")
        self.assertEqual(missing + unmapped, [])
        by_name = {row.name: row for row in rows}
        self.assertTrue(by_name["LEFT"].use_contexts[0].endswith(":argument:1"))
        self.assertTrue(by_name["RIGHT"].use_contexts[0].endswith(":argument:2"))

    def test_macro_const_and_enum_share_destination_but_not_overloads(self):
        rows, _, missing, unmapped, _ = self.scan(
            "enum Domain { VALUE=7 };\n#define ALIAS (VALUE)\n"
            "const int named=VALUE;\n"
            "void consume(int); void consume(long);\n"
            "struct Other { static void consume(int); };\n"
            "void f(){ consume(VALUE); consume(ALIAS); consume(named);\n"
            "consume(static_cast<long>(VALUE)); Other::consume(VALUE); }\n"
        )
        self.assertEqual(missing + unmapped, [])
        by_name = {row.name: row for row in rows}
        direct = set(by_name["ALIAS"].use_contexts) & set(by_name["named"].use_contexts)
        self.assertEqual(len(direct), 1)
        self.assertTrue(next(iter(direct)).startswith("call:"))
        self.assertNotIn("/via:", next(iter(direct)))
        self.assertTrue(direct <= set(by_name["VALUE"].use_contexts))
        self.assertEqual(len([key for key in by_name["VALUE"].use_contexts
                              if key.startswith("call:")]), 3)

    def test_extension_only_appends_pending_without_rewriting_decision(self):
        rows, blocks, _, _, _ = self.scan("enum E { A=9, ADDED=10 }; const int named=9;\n")
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "ledger.tsv"
            original = {"source_enum": "src/Probe.cpp:E", "members": "A=9",
                        "decision": "retain", "current_enums": "src/Probe.cpp:E",
                        "member_reuse": "", "reason": "tested owner contract"}
            reuse._write_tsv(path, reuse.LEDGER_FIELDS, [original])
            self.assertEqual(reuse.extend_ledger(path, rows), 1)
            with path.open() as stream:
                ledger = list(csv.DictReader(stream, dialect="excel-tab"))
            self.assertEqual(ledger[0], original)
            self.assertEqual(ledger[1]["decision"], "pending")
            self.assertEqual(reuse.extend_ledger(path, rows), 0)
            self.assertTrue(any("ADDED" in finding for finding in reuse.check_ledger(path, rows)))

    def test_named_uses_share_field_assignment_comparison_and_switch(self):
        rows, _, missing, unmapped, _ = self.scan(
            "enum E { ASSIGN=3, COMPARE=3, CASE=3 };\n"
            "struct State { int code; };\n"
            "void f(State& s){ s.code=ASSIGN; if(s.code==COMPARE){}\n"
            "switch(s.code){ case CASE: break; } }\n"
        )
        self.assertEqual(missing + unmapped, [])
        keys = [set(row.use_contexts) for row in rows]
        self.assertEqual(len(set.intersection(*keys)), 1)
        self.assertTrue(next(iter(set.intersection(*keys))).startswith("value:"))

    def test_default_parameter_and_runtime_macro_are_not_named_values(self):
        rows, _, missing, unmapped, observations = self.scan(
            "#define OVERRIDE override\n"
            "#define IS_ACTIVE (state != 0)\n"
            "#define CLEAR clear(state)\n"
            "void clear(int&); struct S { int state;\n"
            "void f(const double a=0.0, const double b=0.0) {\n"
            "if(IS_ACTIVE) { CLEAR; } } };\n"
        )
        self.assertEqual(rows, [])
        self.assertEqual(missing + unmapped, [])
        by_name = {row.name: row for row in observations}
        self.assertIn("runtime", by_name["IS_ACTIVE"].reason)
        self.assertIn("non-integral", by_name["CLEAR"].reason)
        self.assertIn("parameter", by_name["a"].reason)
        self.assertIn("keyword", by_name["OVERRIDE"].reason)

    def test_nonintegral_macro_use_witness_covers_other_including_tus(self):
        with tempfile.TemporaryDirectory() as directory:
            repo = Path(directory)
            (repo / "include").mkdir()
            (repo / "src").mkdir()
            (repo / "include/Clear.h").write_text(
                "void* clear(void*, int, unsigned);\n"
                "#define CLEAR clear(buffer, 0, sizeof(buffer))\n")
            (repo / "src/A.cpp").write_text(
                '#include "../include/Clear.h"\n'
                "struct S { char buffer[4]; void f(){ CLEAR; } };\n")
            (repo / "src/B.cpp").write_text('#include "../include/Clear.h"\n')
            (repo / "src/C.cpp").write_text('char buffer[4];\n#include "../include/Clear.h"\n')
            entries = [{"directory": str(repo), "file": f"src/{name}.cpp",
                        "arguments": ["clang-cl", "/c", f"src/{name}.cpp", "/TP"]}
                       for name in ("A", "B", "C")]
            raw, contexts, errors = reuse.scan_entries(entries, repo=repo)
            self.assertEqual(errors, [])
            clear = [row for row in raw.values() if row.name == "CLEAR"]
            self.assertEqual({row.status for row in clear}, {"excluded"})
            self.assertTrue(any(row.reason.startswith("expanded macro") for row in clear))
            _, missing, unmapped = reuse._join(raw, contexts, reuse.scan_blocks(repo=repo))
            self.assertEqual(missing + unmapped, [])

    def test_mixed_context_macro_keeps_numeric_observation(self):
        with tempfile.TemporaryDirectory() as directory:
            repo = Path(directory)
            (repo / "include").mkdir()
            (repo / "src").mkdir()
            (repo / "include/Value.h").write_text("#define VALUE bound\n")
            (repo / "src/A.cpp").write_text(
                'const int bound=7;\n#include "../include/Value.h"\n'
                "int f(){ return VALUE; }\n")
            (repo / "src/B.cpp").write_text(
                '#include "../include/Value.h"\n'
                "struct S { void* bound; void* f(){ return VALUE; } };\n")
            entries = [{"directory": str(repo), "file": f"src/{name}.cpp",
                        "arguments": ["clang-cl", "/c", f"src/{name}.cpp", "/TP"]}
                       for name in ("A", "B")]
            raw, _, errors = reuse.scan_entries(entries, repo=repo)
            self.assertEqual(errors, [])
            value = [row for row in raw.values() if row.name == "VALUE"]
            self.assertIn(("evaluated", 7), {(row.status, row.value) for row in value})
            self.assertIn(("excluded", None), {(row.status, row.value) for row in value})

    def replacement_ledger(self, source, *, expression="sizeof(buffer)", decision="replace",
                           reason="The transfer consumes the complete buffer object."):
        constants, _, _, _, _ = self.scan(source)
        with tempfile.TemporaryDirectory() as directory:
            repo = Path(directory)
            (repo / "src").mkdir()
            (repo / "src/Probe.cpp").write_text(source)
            ledger = repo / "review.tsv"
            reuse._write_tsv(ledger, reuse.LEDGER_FIELDS, [{
                "source_enum": "src/Probe.cpp:Old", "members": "KEEP=1;LIMIT=4",
                "decision": decision, "current_enums": "src/Probe.cpp:Old",
                "member_reuse": "LIMIT=expression:src/Probe.cpp::" + expression,
                "reason": reason,
            }])
            return reuse.check_ledger(ledger, constants, repo=repo)

    def test_expression_replacement_keeps_other_domain_members(self):
        self.assertEqual(self.replacement_ledger(
            "enum Old { KEEP=1 }; int buffer; unsigned f(){return sizeof (buffer);}"), [])

    def test_expression_replacement_rejects_surviving_identifier(self):
        findings = self.replacement_ledger(
            "enum Old { KEEP=1 }; int buffer, LIMIT; unsigned f(){return sizeof(buffer);}")
        self.assertTrue(any("LIMIT still occurs" in finding for finding in findings), findings)
        findings = self.replacement_ledger(
            "enum Old { KEEP=1, LIMIT=4 }; int buffer; unsigned f(){return sizeof(buffer);}")
        self.assertTrue(any("still has a declaration" in finding for finding in findings), findings)

    def test_expression_replacement_requires_real_code_witness_and_review(self):
        findings = self.replacement_ledger(
            'enum Old { KEEP=1 }; // sizeof(buffer)\nconst char* note="sizeof(buffer)";')
        self.assertTrue(any("expression not found" in finding for finding in findings), findings)
        source = "enum Old { KEEP=1 }; int buffer; unsigned f(){return sizeof(buffer);}"
        findings = self.replacement_ledger(source, reason="")
        self.assertTrue(any("evidence reason" in finding for finding in findings), findings)
        findings = self.replacement_ledger(source, decision="reuse")
        self.assertTrue(any("requires replace decision" in finding for finding in findings), findings)

    def test_expression_replacement_checks_token_boundaries(self):
        findings = self.replacement_ledger(
            "enum Old { KEEP=1 }; int buffer; unsigned f(){return sizeof(buffer);}",
            expression="izeof(buffer)")
        self.assertTrue(any("expression not found" in finding for finding in findings), findings)


if __name__ == "__main__":
    unittest.main()
