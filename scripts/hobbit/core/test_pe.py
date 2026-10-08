"""Regression controls for Hobbit adaptations of Gruntz's PE reader."""

import struct
import tempfile
import unittest
from pathlib import Path

from hobbit.core.pe import Pe


def pe_fixture() -> bytes:
    """Minimal PE32 with raw padding and a loader-zero .data tail."""
    blob = bytearray(0x800)
    blob[:2] = b"MZ"
    struct.pack_into("<I", blob, 0x3C, 0x80)
    blob[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<HHIIIHH", blob, 0x84, 0x14C, 3, 0, 0, 0, 0xE0, 0)
    struct.pack_into("<H", blob, 0x98, 0x10B)
    struct.pack_into("<I", blob, 0x98 + 28, 0x400000)
    sections = (
        (b".text", 0x100, 0x1000, 0x200, 0x200),
        (b".rdata", 0x80, 0x2000, 0x200, 0x400),
        (b".data", 0x300, 0x3000, 0x200, 0x600),
    )
    for index, (name, virtual, rva, raw, offset) in enumerate(sections):
        at = 0x178 + index * 40
        blob[at:at + 8] = name.ljust(8, b"\0")
        struct.pack_into("<IIII", blob, at + 8, virtual, rva, raw, offset)
    blob[0x600:0x800] = b"D" * 0x200
    return bytes(blob)


class PeControls(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.path = Path(self.directory.name) / "fixture.exe"
        self.path.write_bytes(pe_fixture())

    def test_optional_import_section_and_raw_padding(self):
        image = Pe(self.path)
        self.assertEqual(image.text_span(), (0x1000, 0x1100))
        self.assertEqual(image.data_regions(), {
            "rdata": (0x2000, 0x2200),
            "data": (0x3000, 0x3200),
            "bss": (0x3200, 0x3300),
        })

    def test_zero_fill_never_reads_adjacent_file_data(self):
        image = Pe(self.path)
        self.assertEqual(image.read(0x31FE, 4), b"DD\0\0")
        self.assertEqual(image.read(0x3200, 8), bytes(8))
        self.assertIsNone(image.read(0x32FF, 2))
        with self.assertRaises(ValueError):
            image.read(0x3000, -1)

    def test_raw_padding_does_not_create_inverted_bss(self):
        blob = bytearray(pe_fixture())
        struct.pack_into("<I", blob, 0x178 + 2 * 40 + 8, 0x80)
        self.path.write_bytes(blob)
        self.assertEqual(Pe(self.path).data_regions()["bss"], (0x3200, 0x3200))

    def test_rejects_malformed_or_truncated_image(self):
        original = pe_fixture()
        payloads = [b"", b"MZ", bytes(64), original[:128], original[:-1]]
        for offset, fmt, value in ((0x3C, "<I", 0xFFFFFF00),
                                   (0x84, "<H", 0x8664),
                                   (0x94, "<H", 2),
                                   (0x98, "<H", 0x20B)):
            blob = bytearray(original)
            struct.pack_into(fmt, blob, offset, value)
            payloads.append(bytes(blob))
        for index, payload in enumerate(payloads):
            with self.subTest(case=index):
                self.path.write_bytes(payload)
                with self.assertRaises(ValueError):
                    Pe(self.path)


if __name__ == "__main__":
    unittest.main()
