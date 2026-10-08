from __future__ import annotations

from collections import Counter
from pathlib import Path
import struct
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock

from hobbit.delink import data_manifest as manifest


FLOAT = struct.pack("<f", -2.0)
DOUBLE = struct.pack("<d", 0.5)
FUNCTION_RVA = 0x1000
FLOAT_RVA = 0x2000
DOUBLE_RVA = FLOAT_RVA + 4


def binding(name, rva, size, channel="src", unit="example"):
    return SimpleNamespace(name=name, rva=rva, size=size, channel=channel,
                           unit=unit)


class PoolObject:
    def __init__(self, code=None, padding=b"\0" * 4, double=True):
        self.code = code if code is not None else b"\xd9\x05" + b"\0" * 4 \
            + b"\xdd\x05" + b"\0" * 4 + b"\xc3"
        self.payload = FLOAT + padding + (DOUBLE if double else b"")
        self.members = [(0, "$T1", 3)]
        if double:
            self.members.append((4 + len(padding), "$T2", 3))
        self.section_table = [
            {"index": 1, "name": ".rdata", "size": len(self.payload),
             "characteristics": 0x40000000, "alignment": 8,
             "comdat": 0, "assoc": 0},
            {"index": 2, "name": ".text", "size": len(self.code),
             "characteristics": manifest.MEM_EXECUTE, "alignment": 16,
             "comdat": 0, "assoc": 0},
        ]
        self.relocations = {2: ("$T1", manifest.COFF_DIR32)}
        if double:
            self.relocations[8] = ("$T2", manifest.COFF_DIR32)

    def section_members(self, index):
        return self.members if index == 1 else [(0, "_example", 2)]

    def defined_symbols(self, index):
        return [(0, "_example")] if index == 2 else []

    def section_payload(self, index):
        return self.payload if index == 1 else self.code

    def typed_relocations(self, index):
        return self.relocations if index == 2 else {}


class PoolImage:
    image_base = 0x400000

    def __init__(self, code=None, payload=FLOAT + DOUBLE, sites=(2, 8)):
        code = code if code is not None else b"\xd9\x05" \
            + struct.pack("<I", self.image_base + FLOAT_RVA) \
            + b"\xdd\x05" \
            + struct.pack("<I", self.image_base + DOUBLE_RVA) + b"\xc3"
        self.data = bytearray(0x3000)
        self.data[FUNCTION_RVA:FUNCTION_RVA + len(code)] = code
        self.data[FLOAT_RVA:FLOAT_RVA + len(payload)] = payload
        self.absolute_sites = [FUNCTION_RVA + site for site in sites]
        self.pe = self

    def off(self, rva):
        return rva if 0 <= rva < len(self.data) else None

    def read(self, rva, size):
        return bytes(self.data[rva:rva + size])

    def classify_storage(self, rva):
        return "rdata" if FLOAT_RVA <= rva < 0x3000 else "other-section"


class PoolExtentTests(unittest.TestCase):
    def pool_rows(self, objects=None, image=None, pins=(), functions=True):
        objects = objects if objects is not None else [("example", PoolObject())]
        model = SimpleNamespace(
            functions=[binding("_example", FUNCTION_RVA,
                               len(objects[0][1].code))] if functions else [],
            data=list(pins))
        with mock.patch.object(manifest.coffx, "objects", return_value=objects), \
                mock.patch.object(manifest, "retail", return_value=image or PoolImage()):
            return manifest.fp_pool_rows(model)

    def test_automatic_width_excludes_padding_beside_double(self):
        rows, withheld = self.pool_rows()
        self.assertEqual(withheld, [])
        self.assertEqual([(r["rva"], r["size"]) for r in rows],
                         [(FLOAT_RVA, 4), (DOUBLE_RVA, 8)])

    def test_stranded_pin_claims_literal_size(self):
        pin = binding(f"$T{FLOAT_RVA}", FLOAT_RVA, 4, "src_data_compgen")
        rows, _withheld = self.pool_rows(pins=[pin], functions=False)
        self.assertEqual([(r["rva"], r["size"]) for r in rows], [(FLOAT_RVA, 4)])
        self.assertEqual(rows[0]["provenance"], "src-DATA_COMPGEN-fp-pool")

    def test_stranded_pin_cannot_claim_prefix_of_different_scalar_type(self):
        pin = binding(f"$T{FLOAT_RVA}", FLOAT_RVA, 4, "src_data_compgen")
        candidate = PoolObject(code=b"\xdd\x05" + b"\0" * 4 + b"\xc3",
                               double=False)
        rows, withheld = self.pool_rows(objects=[("example", candidate)],
                                        pins=[pin], functions=False)
        self.assertEqual(rows, [])
        self.assertTrue(any("width evidence disagrees" in reason
                            for _rva, _name, reason in withheld))

    def test_folded_copies_can_have_different_alignment_padding(self):
        code = b"\xd9\x05" + b"\0" * 4 + b"\xc3"
        padded = PoolObject(code=code, double=False)
        compact = PoolObject(code=code, padding=b"", double=False)
        image = PoolImage(code=b"\xd9\x05"
                          + struct.pack("<I", 0x400000 + FLOAT_RVA) + b"\xc3",
                          sites=(2,))
        rows, withheld = self.pool_rows(objects=[("example", padded),
                                                ("copy", compact)], image=image)
        self.assertEqual(withheld, [])
        self.assertEqual([(r["object"], r["size"]) for r in rows
                          if r["rva"] == FLOAT_RVA],
                         [("example.c", 4), ("copy.c", 4)])

    def test_pin_and_operand_width_contradiction_is_withheld(self):
        pin = binding(f"$T{FLOAT_RVA}", FLOAT_RVA, 8, "src_data_compgen")
        rows, withheld = self.pool_rows(pins=[pin])
        self.assertEqual([r["rva"] for r in rows], [DOUBLE_RVA])
        self.assertTrue(any("width evidence disagrees" in reason
                            for _rva, _name, reason in withheld))

    def test_candidate_and_retail_width_contradiction_is_withheld(self):
        image = PoolImage()
        image.data[FUNCTION_RVA] = 0xDD
        rows, withheld = self.pool_rows(image=image)
        self.assertEqual([r["rva"] for r in rows], [DOUBLE_RVA])
        self.assertTrue(any("width evidence disagrees" in reason
                            for _rva, _name, reason in withheld))

    def test_literal_payload_is_still_verified(self):
        rows, withheld = self.pool_rows(image=PoolImage(payload=b"\0" * 4 + DOUBLE))
        self.assertEqual([r["rva"] for r in rows], [DOUBLE_RVA])
        self.assertTrue(any("bytes contradict" in reason
                            for _rva, _name, reason in withheld))

    def test_opaque_reference_retains_byte_proved_boundary_extent(self):
        candidate = PoolObject(code=b"\xa1" + b"\0" * 4 + b"\xc3",
                               double=False)
        candidate.relocations = {1: ("$T1", manifest.COFF_DIR32)}
        image = PoolImage(code=b"\xa1" + struct.pack("<I", 0x400000 + FLOAT_RVA)
                          + b"\xc3", payload=FLOAT + b"\0" * 4, sites=(1,))
        rows, withheld = self.pool_rows(objects=[("example", candidate)], image=image)
        self.assertEqual(withheld, [])
        self.assertEqual(rows[0]["size"], 8)

    def test_integer_copy_addends_refer_to_one_double(self):
        candidate = PoolObject(code=b"\xa1" + b"\0" * 4 + b"\xa1"
                               + struct.pack("<I", 4) + b"\xc3", double=False)
        candidate.payload = DOUBLE
        candidate.relocations = {1: ("$T1", 6), 6: ("$T1", 6)}
        image = PoolImage(code=b"\xa1" + struct.pack("<I", 0x400000 + FLOAT_RVA)
                          + b"\xa1" + struct.pack("<I", 0x400000 + FLOAT_RVA + 4)
                          + b"\xc3", payload=DOUBLE, sites=(1, 6))
        rows, withheld = self.pool_rows(objects=[("example", candidate)], image=image)
        self.assertEqual(withheld, [])
        self.assertEqual([(r["rva"], r["size"]) for r in rows], [(FLOAT_RVA, 8)])

    def test_section_layout_preserves_zero_padding(self):
        rows, _ = self.pool_rows()
        with tempfile.TemporaryDirectory() as directory:
            (Path(directory) / "example.obj").touch()
            with mock.patch.object(manifest.coffx, "Obj", return_value=PoolObject()):
                sections, placed = manifest.ordinary_sections(rows, Path(directory))
        self.assertEqual(len(placed), 2)
        self.assertEqual(sections[0]["size"], 16)
        self.assertIsNone(sections[0]["rva"])
        self.assertEqual([r["section_offset"] for r in placed], [0, 8])
        self.assertEqual([r["size"] for r in placed], [4, 8])

    def test_nonzero_uncovered_bytes_do_not_become_section_padding(self):
        rows, _ = self.pool_rows()
        with tempfile.TemporaryDirectory() as directory:
            (Path(directory) / "example.obj").touch()
            with mock.patch.object(manifest.coffx, "Obj",
                                   return_value=PoolObject(padding=b"X" * 4)):
                self.assertEqual(manifest.ordinary_sections(rows, Path(directory)),
                                 ([], []))

    def test_pin_and_automatic_row_collapse_without_overlap(self):
        rows, _ = self.pool_rows()
        pin = dict(rows[0], provenance="src_data_compgen")
        del pin["member"]
        with mock.patch.object(manifest, "_candidate_member_storage", return_value={}), \
                mock.patch.object(manifest, "claim_rows", return_value=([pin], [], Counter())), \
                mock.patch.object(manifest, "string_rows", return_value=([], [])), \
                mock.patch("hobbit.delink.empty_strings.rows", return_value=([], [])), \
                mock.patch("hobbit.delink.private_strings.rows", return_value=([], [])), \
                mock.patch("hobbit.delink.real_constants.rows", return_value=([], [])), \
                mock.patch("hobbit.delink.static_guards.rows", return_value=([], [])), \
                mock.patch.object(manifest, "vtable_rows", return_value=([], [])), \
                mock.patch.object(manifest, "rtti_rows", return_value=([], [])), \
                mock.patch.object(manifest, "ehfuncinfo_rows", return_value=([], [])), \
                mock.patch.object(manifest, "fp_pool_rows", return_value=(rows, [])):
            enrolled, withheld, overlaps, _ = manifest.candidates(SimpleNamespace(functions=[], data=[]))
        self.assertEqual((withheld, overlaps), ([], []))
        self.assertEqual(len(enrolled), 2)
        self.assertEqual(enrolled[0]["member"], "$T1")


class ReadWidthTests(unittest.TestCase):
    def test_only_decoded_fp_reads_supply_width(self):
        instructions = [b"\xd9\x05", b"\xd8\x1d", b"\xdd\x05", b"\xdc\x35",
                        b"\xdb\x05", b"\xdf\x2d", b"\xd9\x1d", b"\xdd\x1d"]
        code = b"".join(op + b"\0" * 4 for op in instructions)
        self.assertEqual(manifest._fp_read_widths([code]),
                         [{2: 4, 8: 4, 14: 8, 20: 8}])

    def test_embedded_opcode_bytes_and_truncated_tail_do_not_shift_next_blob(self):
        # D9 05 inside a push-immediate is not an instruction.
        opaque = b"\x68\xd9\x05\0\0\xc3\xd9"
        read = b"\xd9\x05" + b"\0" * 4
        self.assertEqual(manifest._fp_read_widths([opaque, read]), [{}, {2: 4}])


if __name__ == "__main__":
    unittest.main()
