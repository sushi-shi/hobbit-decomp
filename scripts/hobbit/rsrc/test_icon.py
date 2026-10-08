"""Resource staging uses local verified bytes, never distributed media."""

from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch

from hobbit.rsrc.icon import icon_container, stage_script, verified_resources


def resources():
    rows, entries = [], bytearray(struct.pack('<HHH', 0, 1, 9))
    for ordinal in range(1, 10):
        data = bytes([ordinal]) * ordinal
        rows.append((3, ordinal, 1033, 0, data))
        entries.extend(struct.pack('<BBBBHHIH', 16, 16, 0, 0, 1, 32, len(data), ordinal))
    return [*rows, (14, 'IDC_ICON1', 1033, 0, bytes(entries))]


class LocalIcon(unittest.TestCase):
    def test_container_offsets_and_original_image_order(self):
        rows = resources()
        result = icon_container(rows)
        self.assertEqual(struct.unpack_from('<HHH', result), (0, 1, 9))
        cursor = 6 + 9 * 16
        for index, row in enumerate(rows[:-1]):
            size, offset = struct.unpack_from('<II', result, 6 + index * 16 + 8)
            self.assertEqual(offset, cursor)
            self.assertEqual(result[offset:offset + size], row[4])
            cursor += size
        self.assertEqual(cursor, len(result))

    def test_missing_duplicate_mis_sized_and_extra_images_rejected(self):
        for change in ('missing', 'duplicate', 'size', 'extra', 'language'):
            rows = resources()
            if change == 'missing':
                rows.pop(0)
            elif change == 'duplicate':
                rows.insert(0, rows[0])
            elif change == 'size':
                rows[0] = (*rows[0][:4], b'')
            elif change == 'extra':
                rows.insert(0, (3, 10, 1033, 0, b'other'))
            else:
                rows[-1] = (14, 'IDC_ICON1', 0, 0, rows[-1][4])
            with self.subTest(change=change), self.assertRaises(ValueError):
                icon_container(rows)

    def test_staging_keeps_payload_out_of_source(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            src = root / 'source/Meridian.rc'
            src.parent.mkdir()
            src.write_text('IDC_ICON1 ICON "res/meridian.ico"\n')
            stage = stage_script(src, root / 'build/rsrc', resources())
            self.assertEqual(stage.read_bytes(), src.read_bytes())
            self.assertFalse((src.parent / 'res/meridian.ico').exists())
            self.assertEqual((stage.parent / 'res/meridian.ico').read_bytes(),
                             icon_container(resources()))

    def test_explicit_executable_is_verified_before_reading_resources(self):
        with patch('hobbit.rsrc.icon.Pe') as pe, \
             patch('hobbit.rsrc.icon.read_pe_rsrc') as read:
            pe.return_value.data = b'unverified executable'
            pe.return_value.path = Path('arbitrary.exe')
            with self.assertRaises(ValueError):
                verified_resources('arbitrary.exe')
            read.assert_not_called()


if __name__ == '__main__':
    unittest.main()
