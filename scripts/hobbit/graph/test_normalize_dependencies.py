"""Generated Ninja dependencies must invalidate address comparison evidence."""

from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

from hobbit import graph
from hobbit.graph import emit


class NormalizeDependencies(unittest.TestCase):
    def test_emitted_graph_tracks_manifest_producer_and_normalization_modules(self):
        manifest = {"flags": {"release": ["/O2"]}}
        unit = {"unit": "example", "source": "example.cpp", "cflags": ["/O2"]}
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "build.ninja"
            with mock.patch.object(emit, "load_units", return_value=(manifest, [unit])), \
                    mock.patch.object(emit, "prune_orphan_artifacts", return_value=0), \
                    mock.patch.object(emit, "write_toolchain_id"), \
                    mock.patch.object(emit, "emit_link_phase"), \
                    mock.patch.object(emit, "Scanner") as scanner:
                scanner.return_value.headers.return_value = []
                scanner.return_value.scanned.return_value = set()
                emit.emit(path)
            query = subprocess.run(["ninja", "-f", str(path), "-t", "query",
                                    graph.NORMALIZE_STAMP, emit.DATA_MANIFEST],
                                   check=True, capture_output=True, text=True).stdout
        self.assertIn(emit.DATA_MANIFEST, query)
        for module in ("data_boundaries.py", "function_sizes.py", "normalize.py"):
            self.assertIn("scripts/hobbit/compare/" + module, query)
        manifest_query = query.split(emit.DATA_MANIFEST + ":", 1)[1]
        self.assertIn("input: delink", manifest_query)
        self.assertIn(graph.NORMALIZE_STAMP, manifest_query)
        self.assertIn("scripts/hobbit/core/tsv.py", query)

    def test_full_emitted_graph_rebuilds_normalization_after_evidence_changes(self):
        # Run Ninja's real scheduler over the complete emitted graph. Replace
        # commands with inert output writers so this test never compiles or
        # delinks a game image; the production dependency edges remain intact.
        manifest = {"flags": {"release": ["/O2"]}}
        unit = {"unit": "example", "source": "example.cpp", "cflags": ["/O2"]}
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path, producer = root / "fixture.ninja", root / "producer.py"
            producer.write_text(
                "from pathlib import Path\n"
                "import sys\n"
                "rule = sys.argv[1]\n"
                "outputs = sys.argv[2:]\n"
                f"if rule == 'delink': outputs.append({emit.DATA_MANIFEST!r})\n"
                "for name in set(outputs):\n"
                "    path = Path(name)\n"
                "    path.parent.mkdir(parents=True, exist_ok=True)\n"
                f"    if rule == 'delink' and name == {emit.DATA_MANIFEST!r} and path.exists(): continue\n"
                "    path.write_text(rule)\n"
                "with Path('invocations').open('a') as log: log.write(rule + '\\n')\n")
            original_rule = emit.ninja_syntax.Writer.rule

            def fixture_rule(writer, name, command, **kwargs):
                command = f"{shlex.quote(sys.executable)} {shlex.quote(str(producer))} {name} $out"
                return original_rule(writer, name, command, **kwargs)

            with mock.patch.object(emit, "load_units", return_value=(manifest, [unit])), \
                    mock.patch.object(emit, "prune_orphan_artifacts", return_value=0), \
                    mock.patch.object(emit, "write_toolchain_id"), \
                    mock.patch.object(emit, "emit_link_phase"), \
                    mock.patch.object(emit.ninja_syntax.Writer, "rule", fixture_rule), \
                    mock.patch.object(emit, "Scanner") as scanner:
                scanner.return_value.headers.return_value = []
                scanner.return_value.scanned.return_value = set()
                emit.emit(path)

            def ninja(*args):
                return subprocess.run(["ninja", "-f", str(path), *args], cwd=root,
                                      check=True, capture_output=True, text=True).stdout

            outputs = {line.rsplit(": ", 1)[0] for line in ninja("-t", "targets", "all").splitlines()}
            for name in ninja("-t", "inputs", graph.NORMALIZE_STAMP).splitlines():
                self.assertFalse(Path(name).is_absolute())
                if name not in outputs:
                    source = root / name
                    source.parent.mkdir(parents=True, exist_ok=True)
                    source.write_text("input")
            ninja(graph.NORMALIZE_STAMP)
            log = root / "invocations"
            log.write_text("")
            ninja(graph.NORMALIZE_STAMP)
            self.assertEqual(log.read_text(), "")

            for dependency, expected in ((emit.DATA_MANIFEST, ["normalize"]),
                                         ("scripts/hobbit/compare/data_boundaries.py", ["normalize"]),
                                         (emit.RELOC_REFERENTS, ["delink", "normalize"])):
                stamp_time = (root / graph.NORMALIZE_STAMP).stat().st_mtime_ns
                os.utime(root / dependency, ns=(stamp_time + 1, stamp_time + 1))
                ninja(graph.NORMALIZE_STAMP)
                self.assertEqual(log.read_text().splitlines(), expected)
                log.write_text("")
                ninja(graph.NORMALIZE_STAMP)
                self.assertEqual(log.read_text(), "")


class ResourceDependencies(unittest.TestCase):
    def test_resource_edge_tracks_script_and_pinned_local_icon_input(self):
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "resources.ninja"
            with path.open("w") as output, mock.patch.object(emit, "era_rc_available", return_value=True):
                writer = emit.ninja_syntax.Writer(output)
                emit.emit_link_phase(writer, [], enabled=False)
            result = subprocess.run(["ninja", "-f", str(path), "-t", "query", graph.RESOURCE_RES],
                                    check=True, capture_output=True, text=True).stdout
            self.assertIn("src/Apps/Meridian/Meridian.rc", result)
            self.assertNotIn("src/Apps/Meridian/res/meridian.ico", result)
            self.assertIn("build/orig/Meridian.exe", result)
            self.assertIn("config/retail/targets.json", result)
            self.assertIn("scripts/hobbit/rsrc/icon.py", result)
            self.assertIn("hobbit.rsrc check", path.read_text())
            self.assertIn("resources", result)
            self.assertNotIn("build candidate:", path.read_text())


if __name__ == "__main__":
    unittest.main()
