"""Owner-derived EH alignment requires complete payload/reference replay."""
import struct
import unittest

from hobbit.delink.eh_alignment import prove
from hobbit.delink.eh_band import funcinfo_symbol, unwindmap_symbol, unwind_symbol


def fixture(owner='_Owner', *, pointer_type=6, push_opcode=0x68):
    """Real COFF owner -> registration stub -> FuncInfo -> prefix unwind map."""
    payloads = [bytes([push_opcode]) + bytes(4) + b'\xc3',
                b'\xc3\xb8' + bytes(4) + b'\xc3',
                struct.pack('<iI', -1, 0) +
                struct.pack('<IiIiIiI', 0x19930520, 1, 0, 0, 0, 0, 0)]
    names = [(owner, 0, 1, 0x20, 2), ('$Laction', 0, 2, 0, 6),
             ('$Lstub', 1, 2, 0, 6), ('$Trecord', 8, 3, 0, 3),
             ('$Tmap', 0, 3, 0, 3)]
    relocations = [[(1, 2, 6)], [(2, 3, 6)], [(4, 1, pointer_type), (16, 4, 6)]]
    raw = bytearray(20 + 3 * 40)
    sections = []
    for name, payload, rels, flags in zip(
            (b'.text', b'.text$x', b'.xdata$x'), payloads, relocations,
            (0x60300020, 0x60300020, 0x40300040)):
        raw_offset = len(raw)
        raw += payload
        relocation_offset = len(raw)
        raw += b''.join(struct.pack('<IIH', *rel) for rel in rels)
        sections.append(struct.pack('<8sIIIIIIHHI', name, 0, 0, len(payload),
                                    raw_offset, relocation_offset, 0, len(rels), 0, flags))
    symbol_offset = len(raw)
    strings = bytearray(4)
    for name, value, section, typ, storage in names:
        raw += struct.pack('<II', 0, len(strings))
        strings += name.encode() + b'\0'
        raw += struct.pack('<IhHBB', value, section, typ, storage, 0)
    struct.pack_into('<I', strings, 0, len(strings))
    raw += strings
    raw[:20] = struct.pack('<HHIIIHH', 0x14c, 3, 0, symbol_offset, len(names), 0, 0)
    raw[20:140] = b''.join(sections)
    known = {unwind_symbol(owner, 0): 0x401100, unwindmap_symbol(owner): 0x403000}
    payload = bytearray(payloads[2])
    struct.pack_into('<I', payload, 4, 0x401100)
    struct.pack_into('<I', payload, 16, 0x403000)
    rows = [dict(name=unwindmap_symbol(owner), rva=0x3000, size=8),
            dict(name=funcinfo_symbol(owner), rva=0x3008, size=28)]
    read = lambda rva, size: bytes(payload[rva - 0x3000:rva - 0x3000 + size])
    return bytes(raw), rows, known, read, [0x3004, 0x3010]


class EhAlignment(unittest.TestCase):
    def test_complete_structural_records_and_references(self):
        raw, rows, known, read, sites = fixture()
        proofs = prove(raw, rows, known, read, sites)
        self.assertEqual(set(proofs), {r['name'] for r in rows})
        self.assertEqual([proofs[r['name']]['alignment'] for r in rows], [4, 4])
        self.assertEqual(proofs[rows[1]['name']]['section_offset'], 8)
        self.assertEqual(proofs[rows[1]['name']]['reference_offsets'], [8])

    def test_payload_change_rejected(self):
        raw, rows, known, read, sites = fixture()
        def changed(rva, size):
            data = bytearray(read(rva, size))
            data[-1] ^= 1
            return bytes(data)
        self.assertFalse(prove(raw, rows, known, changed, sites))

    def test_missing_extra_and_unknown_references_rejected(self):
        raw, rows, known, read, sites = fixture()
        self.assertFalse(prove(raw, rows, {}, read, sites))
        wrong_targets = {name: value + 4 for name, value in known.items()}
        self.assertFalse(prove(raw, rows, wrong_targets, read, sites))
        self.assertFalse(prove(raw, rows, known, read, []))
        self.assertFalse(prove(raw, rows, known, read, [0x3000, 0x3004, 0x3010, 0x3014]))
        other_names = {name.replace('_Owner', '_Other'): value for name, value in known.items()}
        self.assertFalse(prove(raw, rows, other_names, read, sites))

    def test_unknown_owner_and_broken_owner_chain_rejected(self):
        raw, rows, known, read, sites = fixture()
        wrong_rows = [dict(row, name=row['name'] + 'wrong') for row in rows]
        self.assertFalse(prove(raw, wrong_rows, known, read, sites))
        raw, rows, known, read, sites = fixture(push_opcode=0x69)
        self.assertFalse(prove(raw, rows, known, read, sites))

    def test_malformed_extents_and_relocation_kind_rejected(self):
        raw, rows, known, read, sites = fixture()
        for size in (0, -1, 1000):
            with self.subTest(size=size):
                self.assertFalse(prove(raw, [dict(row, size=size) for row in rows], known, read, sites))
        # The map's pointer must be a complete DIR32 field within its extent.
        self.assertFalse(prove(raw, [dict(rows[0], size=6)], known, read, sites))
        raw, rows, known, read, sites = fixture(pointer_type=7)
        self.assertNotIn(rows[0]['name'], prove(raw, rows, known, read, sites))


if __name__ == '__main__':
    unittest.main()
