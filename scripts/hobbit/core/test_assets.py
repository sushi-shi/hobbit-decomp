"""Local media extraction must reject unpinned inputs and preserve exact bytes."""
import hashlib
import json
from pathlib import Path
import re
import struct
import tempfile
import unittest

from hobbit.core.assets import extract_default_bitmap, extract_sibling_assets
from hobbit.core.inputs import InputError


class AssetTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        (self.root / 'config/retail').mkdir(parents=True)
        data = bytearray(0x208)
        data[:2] = b'MZ'
        struct.pack_into('<I', data, 0x3c, 0x80)
        data[0x80:0x84] = b'PE\0\0'
        struct.pack_into('<HH', data, 0x84, 0x14c, 1)
        struct.pack_into('<H', data, 0x94, 96)
        struct.pack_into('<H', data, 0x98, 0x10b)
        struct.pack_into('<I', data, 0xb4, 0x400000)
        data[0xf8:0x100] = b'.data\0\0\0'
        struct.pack_into('<IIII', data, 0x100, 8, 0x1000, 8, 0x200)
        data[0x200:] = bytes([1, 2, 3, 4, 5, 6, 7, 8])
        self.exe = self.root / 'sample.exe'
        self.exe.write_bytes(data)
        pins = {'game': {'name': 'sample.exe', 'size': len(data), 'sha256': hashlib.sha256(data).hexdigest()}}
        (self.root / 'config/retail/targets.json').write_text(json.dumps(pins))
        self.spec = {'file': 'sample.inc', 'rva': '0x1000', 'width': 4, 'count': 2,
                     'sha256': hashlib.sha256(data[0x200:]).hexdigest()}
        self.manifest = {'default_bitmap': [self.spec], 'sibling_assets': {}}
        self.save_manifest()

    def save_manifest(self):
        (self.root / 'config/retail/local-assets.json').write_text(json.dumps(self.manifest))

    def test_exact_little_endian_initializer_and_noop_write(self):
        paths = extract_default_bitmap(self.exe, self.root)
        values = [int(x, 16) for x in re.findall(r'0x[0-9a-f]+', paths[0].read_text())]
        self.assertEqual(values, [0x04030201, 0x08070605])
        timestamp = paths[0].stat().st_mtime_ns
        extract_default_bitmap(self.exe, self.root)
        self.assertEqual(paths[0].stat().st_mtime_ns, timestamp)

    def test_wrong_executable_rejected_without_outputs(self):
        self.exe.write_bytes(self.exe.read_bytes()[:-1] + b'\xff')
        with self.assertRaises(InputError):
            extract_default_bitmap(self.exe, self.root)
        self.assertFalse((self.root / 'build').exists())

    def test_wrong_payload_hash_rejected_without_outputs(self):
        self.spec['sha256'] = '0' * 64
        self.save_manifest()
        with self.assertRaises(InputError):
            extract_default_bitmap(self.exe, self.root)
        self.assertFalse((self.root / 'build').exists())

    def test_sibling_pins_and_character_literals(self):
        source = self.root / 'sibling.cpp'
        source.write_text("static u8 Wave[] = { 0x0c, 'N', 'O', 'T' };\n")
        spec = {'name': 'Wave', 'type': 'u8', 'file': 'wave.inc', 'width': 1, 'count': 4,
                'sha256': hashlib.sha256(b'\x0cNOT').hexdigest()}
        self.manifest['sibling_assets']['wave'] = {
            'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(), 'arrays': [spec]}
        self.save_manifest()
        paths = extract_sibling_assets('wave', source, self.root)
        self.assertIn('0x0c, 0x4e, 0x4f, 0x54,', paths[0].read_text())
        source.write_text(source.read_text() + '// changed\n')
        with self.assertRaises(InputError):
            extract_sibling_assets('wave', source, self.root)


if __name__ == '__main__':
    unittest.main()
