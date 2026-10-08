import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from hobbit.verify.tu_emission import require_built

NINJA=shutil.which('ninja')

@unittest.skipUnless(NINJA, "ninja required for dependency freshness controls")
class FreshnessTests(unittest.TestCase):
 def fixture(self,root):
  (root/'build').mkdir()
  for path in ('source.cpp','header.hpp','flags.txt','toolchain.id','extractor.py'):(root/path).write_text('one\n')
  (root/'build/build.ninja').write_text('''builddir = build
rule compile
 command = cp source.cpp $out
 restat = 1
rule labels
 command = cp $in $out
 restat = 1
build build/base.obj: compile source.cpp | header.hpp flags.txt toolchain.id
build build/claims.tsv: labels build/base.obj | extractor.py header.hpp flags.txt toolchain.id
''')
  self.build(root)
 def build(self,root):
  subprocess.run([NINJA,'-f','build/build.ninja','build/claims.tsv'],cwd=root,check=True,stdout=subprocess.PIPE)
 def check(self,root):require_built(root,'build/build.ninja',['build/base.obj','build/claims.tsv'],ninja=NINJA)
 def test_fresh_build_is_read_only(self):
  with tempfile.TemporaryDirectory() as directory:
   root=Path(directory);self.fixture(root);before={p.relative_to(root):p.read_bytes() for p in root.rglob('*') if p.is_file()};self.check(root)
   self.assertEqual(before,{p.relative_to(root):p.read_bytes() for p in root.rglob('*') if p.is_file()})
 def test_each_transitive_input_invalidates_even_with_same_symbol_names(self):
  for changed in ('source.cpp','header.hpp','flags.txt','toolchain.id','extractor.py'):
   with self.subTest(changed=changed),tempfile.TemporaryDirectory() as directory:
    root=Path(directory);self.fixture(root);p=root/changed;p.write_text('two\n');latest=max(p.stat().st_mtime_ns for p in root.rglob('*') if p.is_file());os.utime(p,ns=(latest+1000000,latest+1000000))
    with self.assertRaises(ValueError):self.check(root)
    self.assertEqual((root/'build/base.obj').read_text(),'one\n')
 def test_new_object_requires_claim_refresh(self):
  with tempfile.TemporaryDirectory() as directory:
   root=Path(directory);self.fixture(root);p=root/'build/base.obj';p.write_text('two\n');latest=max(p.stat().st_mtime_ns for p in root.rglob('*') if p.is_file());os.utime(p,ns=(latest+1000000,latest+1000000))
   with self.assertRaises(ValueError):self.check(root)
 def test_command_hash_change_is_stale(self):
  with tempfile.TemporaryDirectory() as directory:
   root=Path(directory);self.fixture(root);p=root/'build/build.ninja';p.write_text(p.read_text().replace('cp source.cpp','cp -f source.cpp'))
   with self.assertRaises(ValueError):self.check(root)
 def test_missing_claim_or_build_log_is_not_accepted(self):
  for missing in ('build/claims.tsv','build/.ninja_log'):
   with self.subTest(missing=missing),tempfile.TemporaryDirectory() as directory:
    root=Path(directory);self.fixture(root);(root/missing).unlink()
    with self.assertRaises(ValueError):self.check(root)
 def test_missing_dependency_is_evidence_error(self):
  with tempfile.TemporaryDirectory() as directory:
   root=Path(directory);self.fixture(root);(root/'header.hpp').unlink()
   with self.assertRaises(ValueError):self.check(root)

if __name__=='__main__':unittest.main(verbosity=2)
