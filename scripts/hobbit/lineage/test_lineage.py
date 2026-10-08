from __future__ import annotations

import hashlib
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest import mock

from hobbit.lineage import discovery, ledger


def row(**changes):
    base = {
        "id": "candidate-one",
        "wave": "1",
        "source_commit": "845119c",
        "source_blob": "a" * 40,
        "source_path": "libs/example/example.cpp",
        "source_symbol": "*",
        "hobbit_symbol": "CExample::Run",
        "rva": "0x1234",
        "module": "Example",
        "relation": "direct-family",
        "decision": "pending",
        "reason": "",
        "retail_evidence": "",
        "landed_commit": "",
    }
    base.update(changes)
    return base


class LedgerTests(unittest.TestCase):
    def test_archive_source_identity_requires_matched_tagged_hashes(self):
        archive = "archive-sha256:" + "a" * 64
        payload = "sha256:" + "b" * 64
        self.assertFalse(ledger.validate_rows([row(source_commit=archive, source_blob=payload)]))
        for commit, blob in [
            (archive, "b" * 40),
            ("845119c", payload),
            ("archive-sha256:" + "a" * 63, payload),
            (archive, "sha256:" + "b" * 63),
            ("archive-sha256:" + "A" * 64, payload),
            (archive, "sha256:" + "G" * 64),
        ]:
            with self.subTest(commit=commit, blob=blob):
                self.assertTrue(ledger.validate_rows([row(source_commit=commit, source_blob=blob)]))
        self.assertFalse(ledger.validate_rows([row()]))

    def test_archive_verify_blobs_checks_actual_extracted_payload(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = root / "Shared" / "helpers.h"
            path.parent.mkdir()
            payload = b"// legacy source\r\ninline int value() { return 7; }\r\n\x92"
            path.write_bytes(payload)
            entry = row(
                source_commit="archive-sha256:" + "a" * 64,
                source_blob="sha256:" + hashlib.sha256(payload).hexdigest(),
                source_path="Shared/helpers.h",
            )
            with mock.patch.object(ledger, "_git", side_effect=AssertionError("archive is not Git")):
                self.assertEqual(ledger.verify_blobs(root, [entry]), [])
                changed_archive = dict(entry, source_commit="archive-sha256:" + "b" * 64)
                self.assertEqual(ledger.verify_blobs(root, [changed_archive]), [])
                path.write_bytes(payload + b"changed")
                errors = ledger.verify_blobs(root, [entry])
                self.assertEqual(len(errors), 1)
                self.assertIn("extracted payload drift", errors[0])
                self.assertIn("archive digest not verified here", errors[0])
                self.assertIn(str(path), errors[0])
                path.unlink()
                errors = ledger.verify_blobs(root, [entry])
                self.assertEqual(len(errors), 1)
                self.assertIn("provide the extraction root", errors[0])

    def test_archive_verify_blobs_rejects_malformed_identity_before_reading(self):
        entry = row(source_commit="archive-sha256:" + "a" * 64)
        with mock.patch.object(ledger, "_git", side_effect=AssertionError("must not use Git")):
            errors = ledger.verify_blobs(Path("/unused"), [entry])
        self.assertEqual(len(errors), 1)
        self.assertIn("archive source_blob", errors[0])

    def test_archive_verify_blobs_rejects_paths_outside_extraction_root(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp) / "extracted"
            root.mkdir()
            for source_path in ("../other.h", str(Path(tmp) / "other.h")):
                with self.subTest(source_path=source_path):
                    entry = row(
                        source_commit="archive-sha256:" + "a" * 64,
                        source_blob="sha256:" + "b" * 64,
                        source_path=source_path,
                    )
                    errors = ledger.verify_blobs(root, [entry])
                    self.assertEqual(len(errors), 1)
                    self.assertIn("must stay relative to extraction root", errors[0])

    def test_git_verify_blobs_still_checks_real_git_objects(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            def git(*args):
                return subprocess.run(
                    ["git", "-C", str(root), "-c", "core.hooksPath=/dev/null", *args],
                    check=True, capture_output=True, text=True
                ).stdout.strip()
            git("init", "--quiet")
            path = root / "helpers.h"
            path.write_bytes(b"// original source\n")
            git("add", "helpers.h")
            git("-c", "user.name=Lineage test", "-c", "user.email=lineage@example.invalid",
                "-c", "commit.gpgsign=false", "commit", "--quiet", "-m", "source fixture")
            entry = row(
                source_commit=git("rev-parse", "HEAD"),
                source_blob=git("rev-parse", "HEAD:helpers.h"),
                source_path="helpers.h",
            )
            self.assertFalse(ledger.validate_rows([entry]))
            self.assertEqual(ledger.verify_blobs(root, [entry]), [])
            path.write_bytes(b"// dirty worktree must not replace revision authority\n")
            self.assertEqual(ledger.verify_blobs(root, [entry]), [])
            errors = ledger.verify_blobs(root, [dict(entry, source_blob="a" * 40)])
            self.assertEqual(len(errors), 1)
            self.assertIn("blob drift", errors[0])
            errors = ledger.verify_blobs(root, [dict(entry, source_path="missing.h")])
            self.assertEqual(len(errors), 1)
            self.assertIn("cannot resolve", errors[0])

    def test_non_adoption_requires_controlled_reason_and_retail_evidence(self):
        errors = ledger.validate_rows([row(decision="do-not-take")])
        self.assertTrue(any("controlled reason" in error for error in errors))
        self.assertTrue(any("retail evidence" in error for error in errors))

    def test_dash_is_an_explicit_empty_landed_commit(self):
        rejected = row(
            decision="do-not-take",
            reason="no-retail-owner",
            retail_evidence="no compatible retail owner",
            landed_commit="-",
        )
        self.assertFalse(ledger.validate_rows([rejected]))

    def test_complete_mode_rejects_pending_rows(self):
        self.assertFalse(ledger.validate_rows([row()]))
        self.assertTrue(any("pending decision remains" in error
                            for error in ledger.validate_rows([row()], complete=True)))

    def test_file_level_claim_covers_an_entity_discovery_for_same_blob(self):
        candidate = {
            "source_commit": "845119c",
            "source_blob": "a" * 40,
            "source_path": "libs/example/example.cpp",
            "source_symbol": "CExample::Run",
        }
        self.assertTrue(ledger.covered(candidate, [row()]))
        candidate["source_blob"] = "b" * 40
        self.assertFalse(ledger.covered(candidate, [row()]))

    def test_queue_is_dependency_wave_then_historical_max(self):
        rows = [row(id="late", wave="2", rva=""), row(id="early", wave="1", rva="")]
        self.assertEqual([item["id"] for item in ledger.queue_rows(rows)], ["early", "late"])


class DiscoveryTests(unittest.TestCase):
    def test_git_reader_preserves_legacy_windows_bytes(self):
        completed = subprocess.CompletedProcess(
            args=[], returncode=0, stdout="legacy \x92 source", stderr=""
        )
        with mock.patch("subprocess.run", return_value=completed) as run:
            self.assertEqual(discovery._git(discovery.SRC, "show", "rev:path"),
                             "legacy \x92 source")
        self.assertEqual(run.call_args.kwargs["encoding"], "latin-1")

    def test_normalized_clone_ignores_identifier_and_literal_spelling(self):
        left = " ".join(f"if (value{i}) result{i} += {i};" for i in range(40))
        right = " ".join(f"if (other{i}) output{i} += {i + 100};" for i in range(40))
        self.assertTrue(discovery.structural_clone(left, right))

    def test_short_common_control_flow_is_not_a_clone(self):
        self.assertFalse(discovery.structural_clone("if (x) return 1;", "if (y) return 2;"))

    def test_different_api_vocabulary_is_not_a_clone(self):
        left = " ".join(f"WidgetType::ApplyAlpha(value{i});" for i in range(80))
        right = " ".join(f"FontEngine::LoadGlyph(other{i});" for i in range(80))
        self.assertFalse(discovery.structural_clone(left, right))

    def test_stripped_tree_identity_tracks_source_payloads_and_names(self):
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp)
            path = source / "example.cpp"
            path.write_text("int example() { return 1; }\n")
            identity = discovery.tree_identity(source)
            entry = row(source_commit=identity,
                        source_blob="sha256:" + hashlib.sha256(path.read_bytes()).hexdigest(),
                        source_path=path.name)
            self.assertFalse(ledger.validate_rows([entry]))
            self.assertEqual(ledger.verify_blobs(source, [entry]), [])
            (source / "art.png").write_bytes(b"art is outside the source snapshot")
            self.assertEqual(discovery.tree_identity(source), identity)
            path.write_text("int example() { return 2; }\n")
            self.assertNotEqual(discovery.tree_identity(source), identity)
            errors = ledger.verify_blobs(source, [entry])
            self.assertTrue(any("source tree drift" in error for error in errors))
            self.assertTrue(any("payload drift" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
