"""Controls for the FP-pool width the canonicalizer names a `$T` constant by."""

import struct
import unittest
from types import SimpleNamespace as NS

from hobbit.compare.canonicalize import (
    CoffObject, _definitions, canonicalize_coff, _eh_unwind_labels,
)

MILLI = bytes.fromhex("6f12833a")  # 0.001f
FLD_M32 = b"\xd9\x05"
FLD_M64 = b"\xdd\x05"
FMUL_M32_SIB = b"\xd8\x0c\x85"  # fmul dword [eax*4+disp32]
MOV_EAX_MOFFS = b"\xa1"


def obj(pool: bytes, *operands: bytes) -> bytes:
    """One `.text` whose operands each name `$T1`, the sole `.rdata` datum."""
    code, sites = bytearray(), []
    for opcode in operands:
        code += opcode
        sites.append(len(code))
        code += bytes(4)
    code += b"\xc3"
    rawptr = 20 + 2 * 40
    relptr = rawptr + len(code)
    poolptr = relptr + 10 * len(sites)
    symptr = poolptr + len(pool)
    strings = bytearray(bytes(4))
    symbols = bytearray()
    for name, section, typ, storage in (("$T1", 2, 0, 3), ("_entry", 1, 0x20, 2)):
        symbols += struct.pack("<II", 0, len(strings))
        strings += name.encode("latin1") + b"\0"
        symbols += struct.pack("<IhHBB", 0, section, typ, storage, 0)
    struct.pack_into("<I", strings, 0, len(strings))
    header = struct.pack("<HHIIIHH", 0x14c, 2, 0, symptr, 2, 0, 0)
    text = struct.pack("<8sIIIIIIHHI", b".text", 0, 0, len(code), rawptr,
                       relptr, 0, len(sites), 0, 0x60500020)
    rdata = struct.pack("<8sIIIIIIHHI", b".rdata", 0, 0, len(pool), poolptr,
                        0, 0, 0, 0, 0x40400040)
    relocs = b"".join(struct.pack("<IIH", site, 0, 6) for site in sites)
    return header + text + rdata + bytes(code) + relocs + pool + symbols + strings


def canonical(data: bytes) -> str:
    row = next(row for row in canonicalize_coff(data).rows
               if row.original_name == "$T1")
    return row.canonical_name


class EmptyExecutableDefinitionControls(unittest.TestCase):
    @staticmethod
    def fixture(flags=0x60501020, typ=0x20, value=0):
        symbol_offset = 60
        header = struct.pack('<HHIIIHH', 0x14c, 1, 0, symbol_offset, 1, 0, 0)
        section = struct.pack('<8sIIIIIIHHI', b'.text', 0, 0, 0,
                              symbol_offset, 0, 0, 0, 0, flags)
        symbol = struct.pack('<8sIhHBB', b'_empty', value, 1, typ, 2, 0)
        return header + section + symbol + struct.pack('<I', 4)

    def test_empty_function_preserves_complete_object_without_identity(self):
        original = self.fixture()
        result = canonicalize_coff(original)
        self.assertEqual(result.data, original)
        self.assertEqual(_definitions(CoffObject(result.data)), ())
        self.assertEqual(result.rows, ())

    def test_empty_data_extent_is_still_rejected(self):
        for flags in (0xC0400080, 0xC0400040):
            with self.subTest(flags=flags), self.assertRaises(ValueError):
                canonicalize_coff(self.fixture(flags=flags, typ=0))

    def test_nonzero_function_offset_is_still_rejected(self):
        with self.assertRaises(ValueError):
            canonicalize_coff(self.fixture(value=1))

    def test_typeless_empty_executable_symbol_is_still_rejected(self):
        with self.assertRaises(ValueError):
            canonicalize_coff(self.fixture(typ=0))


class FloatPoolWidthControls(unittest.TestCase):
    def test_zero_tail_read_as_dword_is_the_packed_float(self):
        packed = canonical(obj(MILLI, FLD_M32))
        self.assertEqual(packed, "$anon_f32_3a83126f_0")
        self.assertEqual(canonical(obj(MILLI + bytes(4), FLD_M32)), packed)
        self.assertEqual(
            canonical(obj(MILLI + bytes(4), FLD_M32, FMUL_M32_SIB)), packed)

    def test_zero_tail_read_as_qword_stays_a_double(self):
        self.assertEqual(canonical(obj(MILLI + bytes(4), FLD_M64)),
                         "$anon_f64_000000003a83126f_0")

    def test_operands_that_prove_no_width_leave_the_span_ambiguous(self):
        for operands in ((MOV_EAX_MOFFS,), (FLD_M32, FLD_M64), ()):
            with self.subTest(operands=operands):
                name = canonical(obj(MILLI + bytes(4), *operands))
                self.assertTrue(name.startswith("$anon_data_"), name)


class ExceptionLabelControls(unittest.TestCase):
    def fixture(self, action_addend=0):
        payload=bytearray(144)
        payload[31]=0xb8
        struct.pack_into('<iIiI',payload,100,-1,action_addend,0,0)
        struct.pack_into('<IiIiIiI',payload,116,0x19930520,2,0,0,0,0,0)
        symbols={i:NS(index=i,value=v,section=s,storage_class=c)
                 for i,(v,s,c) in enumerate([(0,1,6),(8,1,6),(30,1,6),
                                            (31,1,6),(16,2,3),(0,2,3)])}
        coff=NS(data=bytes(payload),symbols=symbols,
                sections=[NS(raw_offset=0,raw_size=41),NS(raw_offset=100,raw_size=44)],
                relocations=[NS(section=s,site=p,symbol_index=t,typ=6)
                             for s,p,t in [(1,32,4),(2,24,5),(2,4,1),(2,12,0)]])
        return coff,symbols[3]

    def test_only_map_actions_not_interior_return_label_define_functions(self):
        coff,stub=self.fixture()
        self.assertEqual([s.value for s in _eh_unwind_labels(coff,stub)],[0,8])

    def test_action_addend_cannot_promote_named_symbol_as_entry(self):
        coff,stub=self.fixture(action_addend=1)
        self.assertEqual(_eh_unwind_labels(coff,stub),[])


if __name__ == "__main__":
    unittest.main()
