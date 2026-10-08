from __future__ import annotations

from pathlib import Path
import struct
import tempfile
import unittest

from hobbit.compare.canonicalize import CoffObject, canonicalize_coff
from hobbit.compare.data_boundaries import canonicalize_boundaries
from hobbit.compare.normalize import normalize
from hobbit.core import tsv


def obj(addend=0x40, target=0, typ=6, *, gap=0, aliases=(), undefined=False,
        writable=False, overlap_reloc=False):
    """An absolute address operand and two real adjacent data definitions."""
    code = b"\xb8" + struct.pack("<I", addend) + b"\xc3"
    data = bytes(range(0x40)) + bytes(gap) + b"N" * 0x70
    relocs = struct.pack("<IIH", 1, target, typ)
    if overlap_reloc:
        relocs += struct.pack("<IIH", 2, target, typ)
    symbols = [("_first$S11", 0, 0 if undefined else 2, 0, 3),
               ("_next$S12", 0x40 + gap, 2, 0, 3),
               ("_entry", 0, 1, 0x20, 2)] + list(aliases)
    strings, table = bytearray(bytes(4)), bytearray()
    count = 0
    for name, value, section, symbol_type, storage, *extra in symbols:
        aux = extra[0] if extra else b""
        table += struct.pack("<II", 0, len(strings))
        strings += name.encode("latin1") + b"\0"
        table += struct.pack("<IhHBB", value, section, symbol_type, storage, len(aux) // 18)
        table += aux
        count += 1 + len(aux) // 18
    struct.pack_into("<I", strings, 0, len(strings))
    rawptr, relptr = 100, 100 + len(code)
    dataptr = relptr + len(relocs)
    symptr = dataptr + len(data)
    header = struct.pack("<HHIIIHH", 0x14c, 2, 0, symptr, count, 0, 0)
    text = struct.pack("<8sIIIIIIHHI", b".text", 0, 0, len(code), rawptr,
                       relptr, 0, len(relocs) // 10, 0, 0x60501020)
    section = struct.pack("<8sIIIIIIHHI", b".data" if writable else b".rdata",
                          0, 0, len(data), dataptr, 0, 0, 0, 0,
                          0xC0400040 if writable else 0x40400040)
    return header + text + section + code + relocs + data + table + strings


def claims():
    return [dict(name="_first$S", object="example.c", rva=0x2000,
                 size=0x40, storage="rdata", section_ordinal="2", section_offset=0,
                 provenance="src-DATA-sizeof"),
            dict(name="_next$S", object="example.c", rva=0x2040,
                 size=0x70, storage="rdata", section_ordinal="2", section_offset=0x40,
                 provenance="src-DATA-sizeof")]


def resolved(data):
    coff = CoffObject(data)
    reloc = coff.relocations[0]
    target = coff.symbols[reloc.symbol_index]
    payload = coff.section_bytes(coff.sections[reloc.section - 1])
    addend = struct.unpack_from("<I", payload, reloc.site)[0]
    return target.section, target.value + addend, target.name, addend


def write_manifest(path, rows):
    fields = ["name", "object", "rva", "size", "storage", "section_ordinal",
              "section_offset", "provenance"]
    tsv.write(path, [], fields, rows)


class BoundaryControls(unittest.TestCase):
    def assert_unchanged(self, original=None, rows=None):
        original = obj() if original is None else original
        rows = claims() if rows is None else rows
        self.assertEqual(canonicalize_boundaries(original, "example.c", rows),
                         (original, ()))

    def test_one_past_and_successor_start_share_a_canonical_address(self):
        original = obj()
        normalized, proof = canonicalize_boundaries(original, "example.c", claims())
        direct = obj(addend=0, target=1)
        self.assertEqual(normalized, direct)
        self.assertEqual(resolved(original)[:2], resolved(normalized)[:2])
        self.assertEqual(resolved(normalized)[2:], ("_next$S12", 0))
        self.assertEqual((proof[0].retail_rva, proof[0].original_addend), (0x2040, 0x40))
        before, after = CoffObject(original), CoffObject(normalized)
        self.assertEqual(before.symbols, after.symbols)
        self.assertEqual(before.section_bytes(before.sections[1]),
                         after.section_bytes(after.sections[1]))
        self.assertEqual(canonicalize_boundaries(normalized, "example.c", claims()),
                         (normalized, ()))

    def test_interior_out_of_range_negative_and_successor_offsets_stay_distinct(self):
        for addend, target in ((0, 0), (0x3F, 0), (0x41, 0), (0xFFFFFFFF, 0), (1, 1)):
            with self.subTest(addend=addend, target=target):
                self.assert_unchanged(obj(addend=addend, target=target))

    def test_only_dir32_addresses_are_rewritten(self):
        for typ in (0x14, 7, 0xB, 1):
            with self.subTest(typ=typ):
                self.assert_unchanged(obj(typ=typ))

    def test_manifest_and_candidate_must_both_be_adjacent(self):
        self.assert_unchanged(obj(gap=8))
        rows = claims()
        rows[1]["rva"] += 8
        self.assert_unchanged(rows=rows)
        rows = claims()
        rows[1]["section_offset"] += 8
        self.assert_unchanged(rows=rows)
        rows = claims()
        rows[1]["section_ordinal"] = "3"
        self.assert_unchanged(rows=rows)

    def test_missing_or_wrong_unit_evidence_leaves_reference_unchanged(self):
        self.assert_unchanged(rows=[])
        self.assert_unchanged(rows=claims()[:1])
        rows = claims()
        rows[1]["object"] = "other.c"
        self.assert_unchanged(rows=rows)
        self.assert_unchanged(obj(undefined=True))

    def test_claimed_extent_and_unambiguous_placement_are_required(self):
        for field, value in (("provenance", "provisional-band-gap-zero"),
                             ("provenance", "candidate-COFF-string"), ("size", 0),
                             ("section_ordinal", "-"), ("section_offset", "-"),
                             ("storage", "bss"), ("storage", "data")):
            with self.subTest(field=field, value=value):
                rows = claims()
                rows[0][field] = value
                self.assert_unchanged(rows=rows)

    def test_aliases_and_overlapping_extents_are_rejected(self):
        for row in (dict(claims()[0]), dict(claims()[1]),
                    dict(claims()[0], name="_overlap", rva=0x203F, size=4)):
            with self.subTest(row=row):
                self.assert_unchanged(rows=claims() + [row])
        for alias in (("_alias", 0, 2, 0, 3), ("_inside", 4, 2, 0, 3),
                      ("_nextAlias", 0x40, 2, 0, 3), ("_first$S22", 4, 2, 0, 3)):
            with self.subTest(alias=alias):
                self.assert_unchanged(obj(aliases=[alias]))

    def test_enrolled_section_ordinals_must_be_positive_decimal_integers(self):
        for ordinal in ("invalid", "0", "-1", "2.0", "0x2", 0, -1, None, 2.5, True):
            with self.subTest(ordinal=ordinal):
                rows = claims()
                for row in rows:
                    row["section_ordinal"] = ordinal
                self.assert_unchanged(rows=rows)
        rows = claims()
        for row in rows:
            row["section_ordinal"] = 2
        _normalized, proof = canonicalize_boundaries(obj(), "example.c", rows)
        self.assertEqual(len(proof), 1)

    def test_retail_extents_cannot_leave_the_unsigned_rva_domain(self):
        for start in (-0x80, 0xFFFFFFF0):
            with self.subTest(start=start):
                rows = claims()
                rows[0]["rva"], rows[1]["rva"] = start, start + 0x40
                self.assert_unchanged(rows=rows)

    def test_auxiliary_records_do_not_hide_data_aliases(self):
        metadata = struct.pack("<IHHIHB3s", 0xB0, 0, 0, 0, 0, 0, bytes(3))
        for name in ("_alias", "_first$S22", ".rdata"):
            for value in (0, 4, 0x40):
                if (name, value) == (".rdata", 0):
                    continue
                with self.subTest(name=name, value=value):
                    self.assert_unchanged(obj(aliases=[(name, value, 2, 0, 3, metadata)]))
        _normalized, proof = canonicalize_boundaries(
            obj(aliases=[(".rdata", 0, 2, 0, 3, metadata)]), "example.c", claims())
        self.assertEqual(len(proof), 1)
        self.assert_unchanged(obj(aliases=[(".rdata", 0, 2, 0, 3, bytes(18))]))
        self.assert_unchanged(obj(aliases=[("_first$S22", 0xB0, 2, 0, 3, metadata)]))

    def test_unbacked_data_sections_cannot_prove_a_boundary(self):
        original = bytearray(obj(writable=True))
        coff = CoffObject(original)
        struct.pack_into("<I", original, coff.sections[1].header_offset + 20, 0)
        rows = claims()
        for row in rows:
            row["storage"] = "data"
        self.assert_unchanged(bytes(original), rows)

    def test_data_payload_differences_are_preserved(self):
        original = bytearray(obj())
        coff = CoffObject(original)
        original[coff.sections[1].raw_offset + 0x40] ^= 1
        coff = CoffObject(original)
        normalized, proof = canonicalize_boundaries(bytes(original), "example.c", claims())
        self.assertEqual(len(proof), 1)
        after = CoffObject(normalized)
        self.assertEqual(coff.section_bytes(coff.sections[1]),
                         after.section_bytes(after.sections[1]))

    def test_successor_content_identity_remains_strict_after_normalization(self):
        base, _proof = canonicalize_boundaries(obj(), "example.c", claims())
        target = bytearray(obj(addend=0, target=1))
        coff = CoffObject(target)
        target[coff.sections[1].raw_offset + 0x40] ^= 1
        base = canonicalize_coff(base).data
        target = canonicalize_coff(bytes(target)).data
        self.assertEqual(resolved(base)[:2], resolved(target)[:2])
        self.assertNotEqual(resolved(base)[2], resolved(target)[2])

    def test_overlapping_relocation_fields_are_not_modified(self):
        self.assert_unchanged(obj(overlap_reloc=True))

    def test_writable_data_boundary_requires_matching_storage(self):
        rows = claims()
        for row in rows:
            row["storage"] = "data"
        _normalized, proof = canonicalize_boundaries(obj(writable=True), "example.c", rows)
        self.assertEqual(len(proof), 1)


class ManifestCacheControls(unittest.TestCase):
    def fixture(self, root, units=("example",)):
        base, target, output, manifest = root / "base", root / "target", root / "out", root / "manifest.tsv"
        base.mkdir()
        target.mkdir()
        rows = []
        for unit in units:
            (base / f"{unit}.obj").write_bytes(obj())
            (target / f"{unit}.c.obj").write_bytes(obj(addend=0, target=1))
            rows += [dict(row, object=unit + ".c") for row in claims()]
        write_manifest(manifest, rows)
        return base, target, output, manifest, rows

    def test_manifest_edit_or_disappearance_rebuilds_from_original_objects(self):
        for deleted in (False, True):
            with self.subTest(deleted=deleted), tempfile.TemporaryDirectory() as tmp:
                base, target, output, manifest, rows = self.fixture(Path(tmp))
                first = normalize(base, target, output, ["example"], data_manifest=manifest, quiet=True)
                self.assertEqual(first["data_boundary_rewrites"], 1)
                unchanged = normalize(base, target, output, ["example"], data_manifest=manifest, quiet=True)
                self.assertEqual(unchanged["skipped"], 2)
                if deleted:
                    manifest.unlink()
                else:
                    rows[1]["rva"] += 4
                    write_manifest(manifest, rows)
                final = normalize(base, target, output, ["example"], data_manifest=manifest, quiet=True)
                self.assertEqual((final["wrote"], final["data_boundary_rewrites"]), (2, 0))
                self.assertEqual(resolved((output / "base/example.obj").read_bytes())[3], 0x40)
                self.assertEqual((base / "example.obj").read_bytes(), obj())

    def test_partial_run_does_not_certify_unvisited_pairs_against_new_manifest(self):
        with tempfile.TemporaryDirectory() as tmp:
            base, target, output, manifest, rows = self.fixture(Path(tmp), ("example", "other"))
            normalize(base, target, output, ["example", "other"], data_manifest=manifest, quiet=True)
            old_digest = (output / "data_boundaries/other.sha256").read_text()
            rows[-1]["rva"] += 4
            write_manifest(manifest, rows)
            normalize(base, target, output, ["example"], data_manifest=manifest, quiet=True)
            self.assertEqual((output / "data_boundaries/other.sha256").read_text(), old_digest)
            self.assertNotEqual((output / "data_boundaries/example.sha256").read_text(), old_digest)
            final = normalize(base, target, output, ["other"], data_manifest=manifest, quiet=True)
            self.assertEqual((final["wrote"], final["data_boundary_rewrites"]), (2, 0))
            self.assertEqual(resolved((output / "base/other.obj").read_bytes())[3], 0x40)

    def test_lost_counterpart_after_partial_run_removes_boundary_rewrite(self):
        for lost_side in ("base", "target"):
            with self.subTest(lost_side=lost_side), tempfile.TemporaryDirectory() as tmp:
                base, target, output, manifest, rows = self.fixture(Path(tmp), ("example", "other"))
                # Rewrite both sides, so losing either counterpart must restore
                # the surviving copy from raw COFF even after a partial run.
                (target / "other.c.obj").write_bytes(obj())
                normalize(base, target, output, ["example", "other"], data_manifest=manifest, quiet=True)
                rows[-1]["rva"] += 4
                write_manifest(manifest, rows)
                normalize(base, target, output, ["example"], data_manifest=manifest, quiet=True)
                path = base / "other.obj" if lost_side == "base" else target / "other.c.obj"
                path.unlink()
                final = normalize(base, target, output, ["other"], data_manifest=manifest, quiet=True)
                self.assertEqual(final["wrote"], 1)
                path = output / ("target/other.c.obj" if lost_side == "base" else "base/other.obj")
                self.assertEqual(resolved(path.read_bytes())[3], 0x40)
                self.assertFalse((output / "data_boundaries/other.sha256").exists())
                missing = output / ("base/other.obj" if lost_side == "base" else "target/other.c.obj")
                self.assertFalse(missing.exists())
                missing = missing.with_name("other.symbols.tsv")
                self.assertFalse(missing.exists())


if __name__ == "__main__":
    unittest.main()
