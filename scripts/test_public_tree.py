"""Synthetic Git fixtures; never use original game artifacts as test data."""

from pathlib import Path
import shutil
import tempfile
import unittest
from unittest.mock import patch

import check_public_tree as guard


class PublicTreeTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.repo = Path(self.temp.name)
        self.git("init", "-q", "-b", "main")
        self.git("config", "user.name", "Fixture")
        self.git("config", "user.email", "fixture@example.invalid")
        self.git("config", "commit.gpgsign", "false")

    def git(self, *args, input=None):
        return guard.git(self.repo, *args, input=input)

    def write(self, name, data):
        path = self.repo / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        self.git("add", "--", name)

    def commit(self, message="synthetic fixture"):
        self.git("commit", "-qm", message)

    def findings(self, staged=False):
        return guard.audit(self.repo, staged)[0]

    def test_empty_repository_and_clean_index(self):
        self.assertEqual(self.findings(), {})
        self.write("source.cpp", b"int example() { return 1; }\n")
        self.assertEqual(self.findings(staged=True), {})
        self.commit()
        self.assertEqual(self.findings(), {})

    def test_deleted_historical_binary_is_still_detected(self):
        self.write("payload.txt", b"MZsynthetic fixture\n")
        self.commit()
        self.git("rm", "payload.txt")
        self.commit()
        self.assertIn("binary or asset content", self.findings()["payload.txt"])

    def test_deleted_assembly_dump_and_notes_remain_forbidden(self):
        names = ("function.asm", "function.asm.inc", "function.disasm.txt",
                 "src/IMPORT-NOTES.md", "config/evidence/replay.json")
        for name in names:
            self.write(name, b"synthetic fixture\n")
        self.commit()
        self.git("rm", "--", *names)
        self.commit()
        self.assertTrue(set(names).issubset(self.findings()))
        self.assertEqual(guard.path_reasons("src/xsc_vm_disasm.cpp"), set())
        self.assertEqual(guard.path_reasons("scripts/hobbit/sema/disasm.py"), set())

    def test_every_historical_name_is_checked_for_reused_blob(self):
        self.write("source.cpp", b"synthetic text\n")
        self.commit()
        self.git("mv", "source.cpp", "original.exe")
        self.commit()
        self.git("mv", "original.exe", "safe.txt")
        self.commit()
        self.assertIn("original.exe", self.findings())

    def test_staged_content_not_worktree_is_audited(self):
        self.write("payload.txt", b"XBEHsynthetic fixture\n")
        (self.repo / "payload.txt").write_text("clean worktree\n")
        self.assertEqual(self.findings(), {})
        self.assertIn("payload.txt", self.findings(staged=True))

    def test_other_local_branch_is_checked(self):
        self.write("safe.txt", b"safe\n")
        self.commit()
        self.git("checkout", "-qb", "old")
        self.write("original.iso", b"synthetic fixture\n")
        self.commit()
        self.git("checkout", "-q", "main")
        self.assertIn("original.iso", self.findings())

    def test_game_urls_and_bare_locators_are_blocked(self):
        locators = [
            b"https://" + b"archive" + b".org/download/example",
            b"archive" + b".org/details/example",
            b"https://github.com/" + b"hobbit-kingdom/" + b"hobbit-versions/releases",
            b"hobbit-kingdom/" + b"hobbit-versions",
            b"www." + b"myabandon" + b"ware.com/game/example",
        ]
        for locator in locators:
            with self.subTest(locator=locator):
                self.assertIn("game acquisition locator", guard.content_reasons(locator))

    def test_deleted_link_and_commit_message_are_checked(self):
        locator = b"archive" + b".org/details/synthetic-fixture"
        self.write("notes.md", locator)
        self.commit(locator.decode())
        self.git("rm", "notes.md")
        self.commit()
        findings = self.findings()
        self.assertIn("game acquisition locator", findings["notes.md"])
        self.assertTrue(any(key.startswith("commit ") for key in findings))

    def test_sdk_and_compiler_source_links_are_allowed(self):
        text = (b"https://github.com/hobbit-kingdom/Hobbit-Kingjoyer-Injected-Edition\n"
                b"https://github.com/sushi-shi/gruntz-decomp/releases/download/tools/compiler.zip\n"
                b"https://github.com/encounter/objdiff\n")
        self.assertEqual(guard.content_reasons(text), set())

    def test_magic_and_extensions(self):
        for magic in guard.MAGIC:
            with self.subTest(magic=magic):
                self.assertIn("binary or asset content", guard.content_reasons(magic + b"fixture"))
        for suffix in guard.FORBIDDEN_SUFFIXES:
            with self.subTest(suffix=suffix):
                self.assertTrue(guard.path_reasons("old/fixture" + suffix.upper()))
        for path in guard.LOCAL_ASSET_PATHS:
            self.assertIn("original embedded asset include", guard.path_reasons(path))

    def test_size_limit_without_allocating_large_fixture(self):
        self.write("large.txt", b"x" * 2048)
        self.commit()
        with patch.object(guard, "MAX_BYTES", 1024):
            self.assertIn("object is at least 100 MiB", self.findings()["large.txt"])

    def test_scanner_and_tests_do_not_trigger_themselves(self):
        for source in (Path(guard.__file__), Path(__file__)):
            shutil.copyfile(source, self.repo / source.name)
            self.git("add", source.name)
        self.assertEqual(self.findings(staged=True), {})

    def test_tagged_blob_is_checked(self):
        self.write("safe.txt", b"safe\n")
        self.commit()
        oid = self.git("hash-object", "-w", "--stdin", input=b"MZsynthetic fixture").decode().strip()
        self.git("tag", "fixture-blob", oid)
        self.assertIn(f"blob {oid}", self.findings())

    def test_tagged_tree_paths_are_checked(self):
        self.write("safe.txt", b"safe\n")
        self.commit()
        asset = next(iter(guard.LOCAL_ASSET_PATHS))
        self.write(asset, b"synthetic source bytes\n")
        tree = self.git("write-tree").decode().strip()
        self.git("tag", "fixture-tree", tree)
        self.git("reset", "--hard", "HEAD")
        self.assertIn(asset, self.findings())

    def test_annotated_tag_message_is_checked(self):
        self.write("safe.txt", b"safe\n")
        self.commit()
        locator = "archive" + ".org/details/synthetic-fixture"
        self.git("tag", "-a", "fixture", "-m", locator)
        self.assertTrue(any(key.startswith("tag ") for key in self.findings()))

    def test_shallow_history_is_rejected(self):
        self.write("safe.txt", b"safe\n")
        self.commit()
        (self.repo / ".git" / "shallow").write_bytes(self.git("rev-parse", "HEAD"))
        with self.assertRaisesRegex(ValueError, "full history required"):
            self.findings()

    def test_replace_ref_cannot_hide_original_blob(self):
        self.write("payload.txt", b"MZsynthetic fixture")
        self.commit()
        original = self.git("rev-parse", "HEAD:payload.txt").decode().strip()
        replacement = self.git("hash-object", "-w", "--stdin", input=b"safe").decode().strip()
        self.git("replace", original, replacement)
        self.assertIn("binary or asset content", self.findings()["payload.txt"])


if __name__ == "__main__":
    unittest.main()
