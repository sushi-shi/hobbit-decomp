import json
import sys
import tempfile
import unittest
from pathlib import Path

from hobbit.core import usage


class UsageLogTests(unittest.TestCase):
    def run_logged(self, dispatch, argv, **kw):
        with tempfile.TemporaryDirectory() as td:
            log = Path(td) / "build" / "hobbit_usage.log"
            try:
                rc = usage.run_logged(dispatch, argv, log, **kw)
            except SystemExit as exc:
                rc = exc.code
            lines = log.read_text().splitlines()
            records = [json.loads(line) for line in
                       log.with_suffix(".jsonl").read_text().splitlines()]
        return rc, lines, records

    def test_success_records_command_without_output(self):
        def dispatch(argv):
            print("routine output")
            return 0
        rc, lines, [record] = self.run_logged(dispatch, ["match", "fader"])
        self.assertEqual(rc, 0)
        self.assertTrue(lines[0].endswith("[0]: hobbit match fader"))
        self.assertEqual((record["outcome"], record["error"]), ("success", None))

    def test_child_compile_error_is_captured_and_classified(self):
        def dispatch(argv):
            return usage.run_process(
                [sys.executable, "-c",
                 "import sys; print('x.cpp(3) : error C2065: bad', file=sys.stderr);"
                 " sys.exit(1)"], cwd=Path.cwd())
        rc, _lines, [record] = self.run_logged(dispatch, ["build"])
        self.assertEqual(rc, 1)
        self.assertEqual(record["error_category"], "compile.cpp")
        self.assertIn("C2065", record["error"])

    def test_query_difference_is_not_an_error(self):
        rc, _lines, [record] = self.run_logged(lambda argv: 1, ["walls", "diagnose"],
                                               failure_rc=2)
        self.assertEqual((rc, record["outcome"]), (1, "difference"))

    def test_exception_is_an_error_with_traceback(self):
        def dispatch(argv):
            raise ValueError("boom")
        rc, _lines, [record] = self.run_logged(dispatch, ["model"])
        self.assertEqual(rc, 2)
        self.assertEqual(record["error_category"], "tool.exception")


if __name__ == "__main__":
    unittest.main()
