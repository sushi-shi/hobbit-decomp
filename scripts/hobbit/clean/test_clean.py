"""Controls for lexical cleanup, reproducible inputs and safe output replacement."""

import json
from pathlib import Path
import subprocess
import tempfile
import unittest

from hobbit.clean import MARKER, generate, snapshot, validate_output, write_output
from hobbit.clean.lexer import ARITY, clean_source, tokens


class SourceCleanup(unittest.TestCase):
    def test_original_data_identifiers_survive(self):
        text = '#ifndef DATA_VAULT_HPP\n#define DATA_VAULT_HPP\nenum { DATA_TYPE_NONE = 0 };\n#endif\n'
        self.assertEqual(clean_source(text), text)

    def test_literals_and_token_boundaries(self):
        source = '''#include <rva.h>
RVA(0x123, 4) int/**/value = DATA_COMPGEN(0x234, call(1, (2 + 3)));
const char *text = "RVA(1, 2) // not a comment";
void f() OVERRIDE;
// removed
'''
        cleaned = clean_source(source)
        self.assertNotIn('#include <rva.h>', cleaned)
        self.assertIn('int value = call(1, (2 + 3));', cleaned)
        self.assertIn('"RVA(1, 2) // not a comment"', cleaned)
        self.assertNotIn('OVERRIDE', cleaned)
        self.assertNotIn('// removed', cleaned)

    def test_compiler_generated_names_and_message_maps(self):
        cleaned = clean_source('''RVA_COMPGEN(1, 2, ??1?$CLTList@UWwdRegion@@@@QAE@XZ)
RVA_DYNINIT(1, 2, owner)
DATA_MESSAGE_MAP(1, 2) BEGIN_MESSAGE_MAP(CThing, CWnd)
DATA(3) int thing;
''')
        self.assertIn('BEGIN_MESSAGE_MAP(CThing, CWnd)', cleaned)
        self.assertIn('int thing;', cleaned)
        self.assertFalse(any(kind == 'word' and word.startswith(('RVA', 'DATA'))
                             for kind, word in tokens(cleaned)))

    def test_line_splicing_and_license(self):
        source = '// hidden \\\nalso hidden\nint value; /\\\n* comment */\n/* Copyright owner */\n'
        cleaned = clean_source(source)
        self.assertNotIn('hidden', cleaned)
        self.assertNotIn('comment', cleaned)
        self.assertIn('/* Copyright owner */', cleaned)

    def test_malformed_or_unknown_annotations_fail(self):
        for source in ('RVA(1)', 'DATA(1, 2)', 'DATA_COMPGEN(1, call(2)',
                       'RVA_FUTURE(1)', 'HOBBIT_EMIT_META', '/* unterminated', '"unterminated'):
            with self.subTest(source=source), self.assertRaises(ValueError):
                clean_source(source)


class OutputSafety(unittest.TestCase):
    def test_protected_paths_and_unmarked_directories(self):
        with tempfile.TemporaryDirectory() as directory:
            repo = Path(directory) / 'repo'
            repo.mkdir()
            for path in (repo, repo.parent, repo / 'src', repo / 'build'):
                with self.subTest(path=path), self.assertRaises(ValueError):
                    validate_output(repo, path)
            existing = repo / 'build/existing'
            existing.mkdir(parents=True)
            (existing / 'mine').write_text('keep')
            with self.assertRaises(ValueError):
                write_output(repo, existing, {'new': b'new'}, 'commit')
            self.assertEqual((existing / 'mine').read_text(), 'keep')

    def test_replacement_git_and_symlink_guards(self):
        with tempfile.TemporaryDirectory() as directory:
            repo = Path(directory) / 'repo'
            repo.mkdir()
            output = repo / 'build/source'
            write_output(repo, output, {'old': b'old'}, 'first')
            write_output(repo, output, {'new': b'new'}, 'second')
            self.assertFalse((output / 'old').exists())
            self.assertEqual(json.loads((output / MARKER).read_text())['commit'], 'second')
            symlink = repo / 'build/link'
            symlink.symlink_to(output, target_is_directory=True)
            with self.assertRaises(ValueError):
                validate_output(repo, symlink / 'nested')
            (output / 'nested').mkdir()
            (output / 'nested/.git').write_text('gitdir: elsewhere')
            with self.assertRaises(ValueError):
                validate_output(repo, output)


class SnapshotInputs(unittest.TestCase):
    def test_committed_snapshot_and_unignored_working_preview(self):
        with tempfile.TemporaryDirectory() as directory:
            repo = Path(directory)
            def git(*args):
                return subprocess.check_output(['git', '-C', str(repo), *args], stderr=subprocess.DEVNULL)
            git('init')
            git('config', 'user.name', 'Export Test')
            git('config', 'user.email', 'test@example.invalid')
            (repo / 'tracked').write_text('committed')
            git('add', 'tracked')
            git('commit', '-m', 'input')
            (repo / 'tracked').write_text('working')
            (repo / 'untracked').write_text('excluded')
            (repo / 'staged').write_text('included')
            git('add', 'staged')
            _, committed = snapshot(repo, 'HEAD')
            _, working = snapshot(repo, 'HEAD', True)
            self.assertEqual(committed, {'tracked': b'committed'})
            self.assertEqual(working, {'tracked': b'working', 'staged': b'included', 'untracked': b'excluded'})

    def test_real_export_is_deterministic_and_standalone(self):
        repo = Path(__file__).resolve().parents[3]
        _, files = snapshot(repo, 'HEAD', True)
        # Older/local snapshots may still have media beside the script. The
        # export allowlist must never propagate those original payloads.
        files['src/Apps/Meridian/res/meridian.ico'] = b'private media fixture'
        first = generate(files)
        self.assertEqual(first, generate(files))
        for name in ('include/rva.h', 'config/match_baseline.tsv', 'scripts/hobbit/cli.py',
                     'AGENTS.md', 'src/Apps/Meridian/res/Meridian.exe',
                     'src/Apps/Meridian/res/meridian.ico'):
            self.assertNotIn(name, first)
        for name in ('src/Apps/Meridian/Meridian.rc',
                     'scripts/hobbitbuild/rsrc/icon.py', 'config/retail/targets.json',
                     'nix/toolchain.nix',
                     'scripts/hobbitbuild/core/paths.py', 'build.py'):
            self.assertIn(name, first)
        for name in ('imports/mss32.c', 'imports/smackw32.c', 'nix/runtime.nix',
                     'play.py', 'scripts/hobbitbuild/play.py'):
            self.assertNotIn(name, first)
        self.assertIsNone(json.loads(first['build.json'])['link'])
        for name, data in first.items():
            if name.startswith(('src/', 'include/')) and Path(name).suffix in ('.h', '.cpp', '.c'):
                self.assertFalse(any(kind == 'word' and (word in (*ARITY, 'OVERRIDE')
                                      or word.startswith('RVA_'))
                                     for kind, word in tokens(data.decode())), name)
            if name.startswith('vendor/'):
                self.assertEqual(data, files[name], name)
        units = json.loads(first['build.json'])['units']
        self.assertTrue(units)
        self.assertTrue(all(unit['source'] in first for unit in units))
        self.assertFalse(any('/test' in name or name.startswith('docs/') for name in first))
        self.assertEqual({name for name in first if name.startswith('config/')},
                         {'config/retail/targets.json', 'config/retail/local-assets.json'})


if __name__ == '__main__':
    unittest.main()
