"""Fresh graphs must generate private media before compile and claim extraction."""
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

from hobbit.graph import emit


class BitmapGraphTests(unittest.TestCase):
    def test_missing_generated_includes_are_declared_inputs(self):
        unit = {'unit': 'aux_Bitmap_DefaultBMP',
                'source': 'src/xCore/Auxiliary/Bitmap/DefaultBMP.cpp', 'cflags': ['/c']}
        targets = ['build/objdiff/base/aux_Bitmap_DefaultBMP.obj',
                   'build/gen/claims/aux_Bitmap_DefaultBMP.tsv', *emit.BITMAP_ASSETS]
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'build.ninja'
            with patch.object(emit.Scanner, 'headers', return_value=[]), \
                 patch.object(emit.Scanner, 'scanned', return_value=set()), \
                 patch.object(emit, 'resource_dependencies', return_value=[]), \
                 patch.object(emit, 'load_units', return_value=({'flags': {'release': ['/c']}}, [unit])), \
                 patch.object(emit, 'prune_orphan_artifacts', return_value=0), \
                 patch.object(emit, 'write_toolchain_id'), \
                 patch.object(emit, 'emit_link_phase'):
                emit.emit(path)
            # Let Ninja interpret continuations and separate real input edges
            # from output references; formatting is not the dependency contract.
            queries = {
                target: subprocess.run(['ninja', '-f', str(path), '-t', 'query', target],
                                       check=True, capture_output=True, text=True).stdout
                for target in targets
            }
        for target in targets[:2]:
            inputs = queries[target].split('  outputs:', 1)[0]
            for asset in emit.BITMAP_ASSETS:
                self.assertIn(asset, inputs)
        for asset in emit.BITMAP_ASSETS:
            inputs = queries[asset].split('  outputs:', 1)[0]
            self.assertIn('input: bitmap_assets', inputs)
            self.assertIn('build/orig/Meridian.exe', inputs)
            self.assertIn('config/retail/local-assets.json', inputs)


if __name__ == '__main__':
    unittest.main()
