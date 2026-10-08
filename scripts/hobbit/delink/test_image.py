"""Target-independent controls for /FIXED admission and PDB section placement."""
import hashlib
import io
import struct
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

from hobbit.delink.image import Image
from hobbit.delink import pdb_synth, scope


def fixture():
    data = bytearray(0x500)
    struct.pack_into('<I', data, 0x3c, 0x80)
    opt = 0x80 + 24
    struct.pack_into('<I', data, opt + 36, 0x200)
    struct.pack_into('<I', data, opt + 92, 16)
    # Deliberately reorder sections: no fixed PDB segment numbers are valid.
    sections = [dict(name='.data', va=0x3000, vsize=0x100, rsize=0x100, rptr=0x400),
                dict(name='.text', va=0x1000, vsize=0x100, rsize=0x100, rptr=0x200),
                dict(name='.rdata', va=0x2000, vsize=0x100, rsize=0x100, rptr=0x300)]
    struct.pack_into('<I', data, 0x210, 0x402020)
    pe = SimpleNamespace(data=bytes(data), image_base=0x400000, sections=sections, path=Path('fixture.exe'))
    pe.section = lambda name: next(s for s in sections if s['name'] == name)
    return Image(pe)


class FixedImageControls(unittest.TestCase):
    def test_no_directory_remains_no_real_relocations(self):
        self.assertEqual(fixture().reloc_sites, [])

    def test_reviewed_manifest_requires_image_hash_and_valid_fields(self):
        image = fixture()
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'absolute.tsv'
            pin = '# image-sha256: ' + hashlib.sha256(image.data).hexdigest() + '\n'
            path.write_text(pin + 'site_rva\tkind\n0x1010\tdir32\n')
            self.assertEqual(image.reviewed_sites(path), [0x1010])
            self.assertEqual(image.reloc_sites, [])
            for body in ('site_rva\tkind\n0x1010\tdir32\n',
                         pin + 'site_rva\tkind\n0x1010\tdir32\n0x1010\tdir32\n',
                         pin + 'site_rva\tkind\n0x10fe\tdir32\n',
                         pin + 'site_rva\tkind\n0x1014\tdir32\n'):
                path.write_text(body)
                with self.assertRaises(ValueError):
                    image.reviewed_sites(path)

    def test_raw_read_cannot_cross_section_or_read_virtual_tail(self):
        image = fixture()
        self.assertIsNone(image.raw_read(0x10fe, 4))
        self.assertIsNone(image.raw_read(0x3100, 4))

    def test_iat_record_uses_actual_rdata_segment(self):
        out = io.StringIO()
        with patch.object(pdb_synth, 'retail', return_value=fixture()):
            pdb_synth.emit_yaml([(0x1000, 1, '_f')], [], [],
                                [(0x2020, '__imp__Foo@0')],
                                {0x1000: ('_f', 'example', 1)}, out)
        text = out.getvalue()
        self.assertIn('RelocSegment:    2', text)
        self.assertIn('Offset:          32\n              Segment:         3', text)

    def test_invalid_model_refused_before_emission(self):
        from hobbit.delink import run, data_manifest
        model = SimpleNamespace(violations=["contradictory source extent"])
        for action in (run.run, pdb_synth.synth, data_manifest.generate):
            with self.subTest(action=action.__module__), self.assertRaisesRegex(ValueError, "invalid model"):
                action(model)

    def test_scope_keeps_all_fields_independent_of_referent(self):
        self.assertEqual(scope.select([0x1004, 0x1010, 0x2000], [(0x1000, 0x1014)]),
                         [0x1004, 0x1010])
        with self.assertRaisesRegex(ValueError, 'crosses selected extent'):
            scope.select([0x1010], [(0x1000, 0x1012)])
        with self.assertRaisesRegex(ValueError, 'crosses selected extent'):
            scope.select([0x1000], [(0x1002, 0x1010)])


if __name__ == '__main__':
    unittest.main()
