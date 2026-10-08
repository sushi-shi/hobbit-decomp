from __future__ import annotations

import os
from pathlib import Path
import struct
import tempfile
import unittest

from hobbit.compare.canonicalize import CoffObject, SYMBOL_SIZE
from hobbit.compare.function_sizes import paired_sizes, _state_sizes
from hobbit.compare.normalize import normalize


# A switch-like inline pointer followed by selectors; the final selector is
# itself 0x90. It must remain part of the complete paired window.
BODY = b"\x90\xc3" + bytes(4) + b"\x01\x02\x90"
SHORT = BODY + b"\x90" * 3
LONG = SHORT + b"\x90" * 4


def obj(code, *, comdat=True, alignment=16, extra=(), relocs=((2, 0, 6),),
        function_aux=b"", lines=()):
    """One executable COFF section with optional aux/line-index evidence."""
    symbols = [("_entry", 0, 1, 0x20, 2, function_aux),
               ("_callee", 0, 0, 0x20, 2, b"")] + list(extra)
    strings, table = bytearray(bytes(4)), bytearray()
    count = 0
    for name, value, section, typ, storage, aux in symbols:
        table += struct.pack("<II", 0, len(strings))
        strings += name.encode("latin1") + b"\0"
        table += struct.pack("<IhHBB", value, section, typ, storage, len(aux) // 18)
        table += aux
        count += 1 + len(aux) // 18
    struct.pack_into("<I", strings, 0, len(strings))
    relocations = b"".join(struct.pack("<IIH", *r) for r in relocs)
    linenos = b"".join(struct.pack("<IH", *line) for line in lines)
    rawptr = 60
    relptr = rawptr + len(code)
    lineptr = relptr + len(relocations)
    symptr = lineptr + len(linenos)
    flags = 0x60000020 | (0x1000 if comdat else 0)
    if alignment:
        flags |= alignment.bit_length() << 20
    header = struct.pack("<HHIIIHH", 0x14c, 1, 0, symptr, count, 0, 0)
    section = struct.pack("<8sIIIIIIHHI", b".text", 0, 0, len(code), rawptr,
                          relptr if relocs else 0, lineptr if lines else 0,
                          len(relocs), len(lines), flags)
    return header + section + code + relocations + linenos + table + strings


def entry(coff):
    return next(s for s in coff.symbols.values() if s.name == "_entry")


def total_size(data):
    coff = CoffObject(data)
    symbol = entry(coff)
    return struct.unpack_from("<I", data, symbol.offset + SYMBOL_SIZE + 4)[0] \
        if symbol.aux_count else None


class FunctionSizeTests(unittest.TestCase):
    def assert_unchanged(self, base, target):
        self.assertEqual(paired_sizes(base, target), (base, target, ()))

    def test_complete_switch_window_and_payloads_are_preserved(self):
        base, target = obj(LONG), obj(SHORT, comdat=False)
        a, b, proofs = paired_sizes(base, target)
        self.assertEqual(len(proofs), 1)
        self.assertEqual((proofs[0].base_span, proofs[0].target_span,
                          proofs[0].size, proofs[0].alignment), (16, 12, 12, 16))
        for original, normalized in ((base, a), (target, b)):
            before, after = CoffObject(original), CoffObject(normalized)
            self.assertEqual(before.sections, after.sections)
            self.assertEqual(before.section_bytes(before.sections[0]),
                             after.section_bytes(after.sections[0]))
            self.assertEqual(total_size(normalized), 12)
            self.assertEqual([s.name for s in before.symbols.values()],
                             [s.name for s in after.symbols.values()])

    def test_packed_window_ends_at_next_function(self):
        next_fn = ("_next", len(SHORT), 1, 0x20, 2, b"")
        a, b, proofs = paired_sizes(obj(LONG), obj(SHORT + b"\xc3", comdat=False,
                                                   extra=[next_fn]))
        self.assertEqual(proofs[0].size, len(SHORT))
        self.assertEqual(total_size(a), len(SHORT))
        coff = CoffObject(b)
        self.assertEqual(coff.section_bytes(coff.sections[0]), SHORT + b"\xc3")

    def test_larger_comdat_may_be_either_side(self):
        a, b, proofs = paired_sizes(obj(SHORT, comdat=False), obj(LONG))
        self.assertEqual(len(proofs), 1)
        self.assertEqual((total_size(a), total_size(b)), (12, 12))

    def test_real_prefix_or_selector_differences_are_not_hidden(self):
        for offset in (0, 6, 8):
            with self.subTest(offset=offset):
                changed = bytearray(LONG)
                changed[offset] ^= 1
                self.assert_unchanged(obj(bytes(changed)), obj(SHORT, comdat=False))

    def test_relocation_identity_type_and_payload_addend_must_agree(self):
        target = obj(SHORT, comdat=False)
        for relocations in (((2, 1, 6),), ((2, 0, 0x14),), ()):
            with self.subTest(relocations=relocations):
                self.assert_unchanged(obj(LONG, relocs=relocations), target)
        changed = bytearray(LONG)
        changed[2] = 1
        self.assert_unchanged(obj(bytes(changed)), target)

    def test_only_terminal_sub_alignment_nops_are_admitted(self):
        target = obj(SHORT, comdat=False)
        for base in (obj(LONG[:-1] + b"\xcc"), obj(LONG + b"\x90" * 16),
                     obj(LONG, comdat=False), obj(LONG, alignment=0),
                     obj(LONG, alignment=4)):
            with self.subTest(base=base):
                self.assert_unchanged(base, target)

    def test_tail_symbol_or_relocation_rejects_padding(self):
        label = ("$L1", 12, 1, 0, 6, b"")
        target = obj(SHORT, comdat=False)
        self.assert_unchanged(obj(LONG, extra=[label]), target)
        self.assert_unchanged(obj(LONG, relocs=((2, 0, 6), (12, 1, 6))), target)

    def test_interior_function_padding_is_not_normalized(self):
        next_fn = ("_next", len(SHORT), 1, 0x20, 2, b"")
        self.assert_unchanged(obj(LONG, extra=[next_fn]), obj(SHORT, comdat=False))

    def test_interior_switch_labels_are_preserved(self):
        label = ("$L1", 6, 1, 0, 6, b"")
        a, b, proofs = paired_sizes(obj(LONG, extra=[label]),
                                   obj(SHORT, comdat=False, extra=[label]))
        self.assertEqual(len(proofs), 1)
        for data in (a, b):
            coff = CoffObject(data)
            symbol = next(s for s in coff.symbols.values() if s.name == "$L1")
            self.assertEqual(symbol.value, 6)

    def test_pointer_into_excluded_tail_rejects_padding(self):
        common = bytearray(SHORT)
        struct.pack_into("<I", common, 2, len(SHORT))
        self.assert_unchanged(obj(bytes(common) + b"\x90" * 4),
                              obj(bytes(common), comdat=False))

    def test_existing_explicit_size_is_not_overwritten(self):
        aux = struct.pack("<IIIIH", 0, 9, 0, 0, 0)
        self.assert_unchanged(obj(LONG, function_aux=aux), obj(SHORT, comdat=False))

    def test_unknown_auxiliary_format_is_not_guessed(self):
        unknown = ("_opaque", 0, 0, 0, 2, bytes(18))
        self.assert_unchanged(obj(LONG, extra=[unknown]), obj(SHORT, comdat=False))

    def test_weak_default_and_relocation_indices_are_remapped(self):
        weak = ("_weak", 0, 0, 0, 105, struct.pack("<IIIIH", 1, 3, 0, 0, 0))
        base, target = obj(LONG, extra=[weak]), obj(SHORT, comdat=False, extra=[weak])
        a, b, _ = paired_sizes(base, target)
        for data in (a, b):
            coff = CoffObject(data)
            weak_symbol = next(s for s in coff.symbols.values() if s.name == "_weak")
            index = struct.unpack_from("<I", data, weak_symbol.offset + SYMBOL_SIZE)[0]
            self.assertEqual(coff.symbols[index].name, "_callee")
            self.assertEqual(coff.symbols[coff.relocations[0].symbol_index].name, "_entry")

    def test_section_aux_and_line_symbol_indices_are_preserved(self):
        section_aux = struct.pack("<IHHIHB3s", 16, 1, 2, 0, 0, 1, bytes(3))
        section = (".text", 0, 1, 0, 3, section_aux)
        data = obj(LONG, extra=[section], lines=[(1, 0), (4, 7)])
        before = CoffObject(data)
        result = _state_sizes(before, {entry(before).index: 12})
        after = CoffObject(result)
        symbol = next(s for s in after.symbols.values() if s.name == ".text")
        self.assertEqual(result[symbol.offset + 18:symbol.offset + 36], section_aux)
        pointer = struct.unpack_from("<I", result, after.sections[0].header_offset + 28)[0]
        index = struct.unpack_from("<I", result, pointer)[0]
        self.assertEqual(after.symbols[index].name, "_callee")
        self.assertEqual(result[pointer + 6:pointer + 12], struct.pack("<IH", 4, 7))

    def test_proven_eh_window_keeps_interior_label_and_states_complete_extent(self):
        name='__ehunwind$_entry$0'
        code=b'\xc3\xb8'+bytes(4)+b'\xc3'
        base=obj(code,extra=[(name,1,1,0,6,b''),('$Lret',6,1,0,6,b'')],relocs=((2,1,6),))
        target=obj(code,extra=[(name,1,1,0x20,2,b'')],relocs=((2,1,6),))
        a,b,proofs=paired_sizes(base,target)
        self.assertEqual([(p.name,p.size,p.proof) for p in proofs],
                         [(name,6,'paired-identical-EH-map-owned-function')])
        c=CoffObject(a)
        helper=next(s for s in c.symbols.values() if s.name==name)
        self.assertEqual((helper.typ,helper.storage_class,helper.aux_count),(0x20,2,1))
        self.assertEqual(struct.unpack_from('<I',a,helper.offset+22)[0],6)
        interior=next(s for s in c.symbols.values() if s.name=='$Lret')
        self.assertEqual((interior.value,interior.storage_class),(6,6))
        self.assertEqual(c.section_bytes(c.sections[0]),code)
        self.assert_unchanged(base,obj(code,extra=[(name,1,1,0x20,2,b'')],relocs=()))
        changed=code[:-1]+b'\xcc'
        self.assert_unchanged(base,obj(changed,extra=[(name,1,1,0x20,2,b'')],relocs=((2,1,6),)))


class PairedCacheTests(unittest.TestCase):
    def test_changing_either_input_removes_a_stale_size_proof_from_both_copies(self):
        for changed_side in ("base", "target"):
            with self.subTest(changed_side=changed_side), tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp)
                base, target, output = root / "base", root / "target", root / "out"
                base.mkdir()
                target.mkdir()
                base_path, target_path = base / "example.obj", target / "example.c.obj"
                base_path.write_bytes(obj(LONG))
                target_path.write_bytes(obj(SHORT, comdat=False))
                first = normalize(base, target, output, ["example"], quiet=True)
                self.assertEqual((first["wrote"], first["function_size_proofs"]), (2, 1))
                second = normalize(base, target, output, ["example"], quiet=True)
                self.assertEqual((second["skipped"], second["function_size_proofs"]), (2, 1))
                changed = bytearray(LONG if changed_side == "base" else SHORT)
                changed[0] ^= 1
                path = base_path if changed_side == "base" else target_path
                path.write_bytes(obj(bytes(changed), comdat=changed_side == "base"))
                # Prove the dependency without sleeps or filesystem clock resolution assumptions.
                latest = max(p.stat().st_mtime for p in output.rglob("*.*"))
                os.utime(path, (latest + 1, latest + 1))
                final = normalize(base, target, output, ["example"], quiet=True)
                self.assertEqual((final["wrote"], final["function_size_proofs"]), (2, 0))
                self.assertIsNone(total_size((output / "base/example.obj").read_bytes()))
                self.assertIsNone(total_size((output / "target/example.c.obj").read_bytes()))

    def test_lost_counterpart_removes_paired_metadata(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            base, target, output = root / "base", root / "target", root / "out"
            base.mkdir()
            target.mkdir()
            (base / "example.obj").write_bytes(obj(LONG))
            target_path = target / "example.c.obj"
            target_path.write_bytes(obj(SHORT, comdat=False))
            normalize(base, target, output, ["example"], quiet=True)
            target_path.unlink()
            normalize(base, target, output, ["example"], quiet=True)
            self.assertIsNone(total_size((output / "base/example.obj").read_bytes()))
            self.assertFalse((output / "target/example.c.obj").exists())
            self.assertFalse((output / "function_sizes/example.tsv").exists())


if __name__ == "__main__":
    unittest.main()
