"""Paired extent proof preserves the inline table and its alignment bytes."""

import struct
import unittest
from unittest.mock import patch

from hobbit.compare.test_function_sizes import obj, total_size
from hobbit.compare.canonicalize import CoffObject
from hobbit.compare.function_sizes import paired_sizes

BODY = b'\xff\x24\x85' + struct.pack('<I', 16) + b'\x90' * 5 + b'\xc3'
CODE = BODY + b'\x8d\x49\x00' + struct.pack('<II', 7, 8) + bytes(4)
TABLES = [('$L1', 16, 1, 0, 3, b''), ('$L2', 24, 1, 0, 3, b'')]
RELOCS = ((3, 0, 6), (16, 0, 6), (20, 0, 6))


def pair(code=CODE, **kwargs):
    base = obj(code, extra=kwargs.pop('extra', TABLES),
               relocs=kwargs.pop('relocs', RELOCS), **kwargs)
    target = obj(BODY, comdat=False, relocs=((3, 0, 6),))
    return base, target


class PreTableTests(unittest.TestCase):
    def unchanged(self, base, target):
        self.assertEqual(paired_sizes(base, target), (base, target, ()))

    def test_metadata_only_complete_body(self):
        base, target = pair()
        left, right, proofs = paired_sizes(base, target)
        self.assertEqual(len(proofs), 1)
        self.assertEqual(proofs[0].size, 13)
        for before, after in [(base, left), (target, right)]:
            old, new = CoffObject(before), CoffObject(after)
            self.assertEqual(old.section_bytes(old.sections[0]),
                             new.section_bytes(new.sections[0]))
            self.assertEqual(total_size(after), 13)
        self.assertEqual(proofs[0].proof,
                         'paired-identical-window-with-pretable-alignment-nop')

    def test_changed_body_or_referent_rejected(self):
        self.unchanged(*pair(CODE[:9] + b'\xcc' + CODE[10:]))
        base, _target = pair()
        self.unchanged(base, obj(BODY, comdat=False, relocs=((3, 1, 6),)))

    def test_executable_suffix_is_not_padding(self):
        self.unchanged(*pair(BODY + b'\x33\xc9\x90' + CODE[16:]))

    def test_table_boundary_must_be_static_compiler_label(self):
        for first in [('$X1', 16, 1, 0, 3, b''),
                      ('$L1', 16, 1, 0, 6, b''),
                      ('_next', 16, 1, 0x20, 2, b'')]:
            with self.subTest(first=first):
                self.unchanged(*pair(extra=[first, TABLES[1]]))

    def test_incomplete_wrong_or_overlapping_table_relocations_rejected(self):
        for relocs in [RELOCS[:-1], ((3, 0, 6), (16, 1, 6), (20, 0, 6)),
                       RELOCS + ((17, 0, 6),)]:
            with self.subTest(relocations=relocs):
                self.unchanged(*pair(relocs=relocs))

    def test_table_extent_must_fit_section(self):
        self.unchanged(*pair(extra=[TABLES[0], ('$L2', 32, 1, 0, 3, b'')]))

    def test_table_pointer_must_land_in_function(self):
        self.unchanged(*pair(CODE[:16] + struct.pack('<I', 15) + CODE[20:]))

    def test_gap_cannot_have_label_or_relocation(self):
        self.unchanged(*pair(extra=TABLES + [('$L3', 13, 1, 0, 6, b'')]))
        self.unchanged(*pair(relocs=RELOCS + ((13, 0, 6),)))

    def test_gap_cannot_be_addressed(self):
        base, _target = pair(CODE[:3] + struct.pack('<I', 14) + CODE[7:])
        target = obj(BODY[:3] + struct.pack('<I', 14) + BODY[7:],
                     comdat=False, relocs=((3, 0, 6),))
        self.unchanged(base, target)

    def test_requires_comdat_and_actual_indexed_jump(self):
        self.unchanged(*pair(comdat=False))
        base, _target = pair(b'\x90' * 3 + CODE[3:])
        target = obj(b'\x90' * 3 + BODY[3:], comdat=False, relocs=((3, 0, 6),))
        self.unchanged(base, target)


MOV_BODY = b'\xff\x24\x85' + struct.pack('<I', 16) + b'\x90' * 6 + b'\xc3'
MOV_CODE = MOV_BODY + b'\x8b\xff' + struct.pack('<II', 7, 8) + bytes(4)
MOV_TABLES = [('$L1', 16, 1, 0, 3, b''), ('$L2', 24, 1, 0, 3, b'')]
MOV_RELOCS = ((3, 0, 6), (16, 0, 6), (20, 0, 6))


def mov_pair(code=MOV_CODE, **kwargs):
    base = obj(code, extra=kwargs.pop('extra', MOV_TABLES),
               relocs=kwargs.pop('relocs', MOV_RELOCS), **kwargs)
    return base, obj(MOV_BODY, comdat=False, relocs=((3, 0, 6),))


class MovPreTableTests(unittest.TestCase):
    def unchanged(self, base, target):
        self.assertEqual(paired_sizes(base, target), (base, target, ()))

    def test_only_metadata_changes_for_complete_body(self):
        base, target = mov_pair()
        left, right, proofs = paired_sizes(base, target)
        self.assertEqual(len(proofs), 1)
        self.assertEqual(proofs[0].size, 14)
        for before, after in [(base, left), (target, right)]:
            old, new = CoffObject(before), CoffObject(after)
            self.assertEqual(old.section_bytes(old.sections[0]),
                             new.section_bytes(new.sections[0]))
            self.assertEqual(total_size(after), 14)
            refs = lambda c: [(r.site, r.typ, c.symbols[r.symbol_index].name)
                              for r in c.relocations]
            self.assertEqual(refs(old), refs(new))

    def test_stack_pop_return_is_decoded_and_preserved(self):
        body = MOV_BODY[:-3] + b"\xc2\x08\x00"
        base = obj(body + MOV_CODE[14:], extra=MOV_TABLES, relocs=MOV_RELOCS)
        target = obj(body, comdat=False, relocs=((3, 0, 6),))
        left, right, proofs = paired_sizes(base, target)
        self.assertEqual(len(proofs), 1)
        self.assertEqual((total_size(left), total_size(right)), (14, 14))
        # The same bytes inside MOV EAX,imm32 do not establish a return.
        body = MOV_BODY[:-5] + b"\xb8\x00\xc2\x08\x00"
        self.unchanged(obj(body + MOV_CODE[14:], extra=MOV_TABLES, relocs=MOV_RELOCS),
                       obj(body, comdat=False, relocs=((3, 0, 6),)))

    def test_final_table_terminal_nops_require_complete_suffix_proof(self):
        # Two table words end at24; terminal COMDAT NOPs align to32.
        code = MOV_CODE[:24] + b"\x90" * 8
        base = obj(code, extra=MOV_TABLES[:1], relocs=MOV_RELOCS)
        target = obj(MOV_BODY, comdat=False, relocs=((3, 0, 6),))
        self.assertEqual(len(paired_sizes(base, target)[2]), 1)
        for altered in (code[:-1] + b"\xcc", code + b"\x90" * 16):
            self.unchanged(obj(altered, extra=MOV_TABLES[:1], relocs=MOV_RELOCS), target)
        self.unchanged(obj(code, extra=MOV_TABLES[:1] + [("_tail", 24, 1, 0, 3, b"")],
                           relocs=MOV_RELOCS), target)
        changed = code[:20] + struct.pack("<I", 24) + code[24:]
        self.unchanged(obj(changed, extra=MOV_TABLES[:1], relocs=MOV_RELOCS), target)
        self.unchanged(obj(code, extra=MOV_TABLES[:1], relocs=MOV_RELOCS + ((24, 1, 6),)), target)
        self.unchanged(obj(code, extra=MOV_TABLES[:1], relocs=MOV_RELOCS[:-1]), target)

    def test_wrong_instruction_or_return_rejected(self):
        for tail in (b'\x8b\xfe', b'\x8b\x3f', b'\x89\xff', b'\x33\xc9'):
            with self.subTest(tail=tail):
                self.unchanged(*mov_pair(MOV_BODY + tail + MOV_CODE[16:]))
        self.unchanged(*mov_pair(MOV_CODE[:13] + b'\x90' + MOV_CODE[14:]))

    def test_body_and_referents_still_match(self):
        self.unchanged(*mov_pair(MOV_CODE[:9] + b'\xcc' + MOV_CODE[10:]))
        base, target = mov_pair()
        self.unchanged(base, obj(MOV_BODY, comdat=False, relocs=((3, 1, 6),)))

    def test_table_structure_and_dispatch_required(self):
        self.unchanged(*mov_pair(comdat=False))
        self.unchanged(*mov_pair(extra=[('$X1', 16, 1, 0, 3, b''), MOV_TABLES[1]]))
        for relocs in (MOV_RELOCS[:-1], ((3, 0, 6), (16, 1, 6), (20, 0, 6)),
                       MOV_RELOCS + ((17, 0, 6),)):
            self.unchanged(*mov_pair(relocs=relocs))
        self.unchanged(*mov_pair(MOV_CODE[:16] + struct.pack('<I', 15) + MOV_CODE[20:]))
        base, target = mov_pair(b'\x90' * 3 + MOV_CODE[3:])
        self.unchanged(base, obj(b'\x90' * 3 + MOV_BODY[3:], comdat=False,
                                relocs=((3, 0, 6),)))

    def test_gap_labels_operands_and_destinations_rejected(self):
        self.unchanged(*mov_pair(extra=MOV_TABLES + [('$L3', 14, 1, 0, 6, b'')]))
        self.unchanged(*mov_pair(relocs=MOV_RELOCS + ((14, 0, 6),)))
        for destination in (14, 15):
            changed = MOV_CODE[:3] + struct.pack('<I', destination) + MOV_CODE[7:]
            target = obj(MOV_BODY[:3] + struct.pack('<I', destination) + MOV_BODY[7:],
                         comdat=False, relocs=((3, 0, 6),))
            self.unchanged(mov_pair(changed)[0], target)


def pair_ending(body, *, relocs=RELOCS):
    return (obj(body + CODE[13:], extra=TABLES, relocs=relocs),
            obj(body, comdat=False, relocs=tuple(r for r in relocs if r[0] < 13)))


class TerminalBranchTests(unittest.TestCase):
    def unchanged(self, base, target):
        self.assertEqual(paired_sizes(base, target), (base, target, ()))

    def test_decoded_local_backward_jump_keeps_full_body(self):
        body = BODY[:-2] + b'\xeb\xfa'
        left, right, proofs = paired_sizes(*pair_ending(body))
        self.assertEqual(len(proofs), 1)
        self.assertEqual((total_size(left), total_size(right)), (13, 13))
        body = BODY[:-5] + b'\xe9' + struct.pack('<i', 7 - 13)
        self.assertEqual(len(paired_sizes(*pair_ending(body))[2]), 1)

    def test_branch_target_must_be_decoded_interior_instruction(self):
        for target in (4, 13, 14, 16, -1):
            with self.subTest(target=target):
                self.unchanged(*pair_ending(BODY[:-2] + b'\xeb' + bytes([(target - 13) & 255])))

    def test_fallthrough_conditional_and_call_are_not_terminal(self):
        for body in (BODY[:-2] + b'\x75\xfa', BODY[:-2] + b'\x90\x90',
                     BODY[:-5] + b'\xe8' + struct.pack('<i', 7 - 13)):
            with self.subTest(body=body):
                self.unchanged(*pair_ending(body))

    def test_embedded_jump_opcode_or_relocated_branch_rejected(self):
        self.unchanged(*pair_ending(BODY[:-5] + b'\xb8\x00\x00\xeb\xfa'))
        body = BODY[:-5] + b'\xe9' + struct.pack('<i', 7 - 13)
        self.unchanged(*pair_ending(body, relocs=RELOCS + ((9, 1, 0x14),)))

    def test_decoder_must_reproduce_entire_raw_body(self):
        from hobbit.sema.disasm import parse_listing
        def corrupt(text):
            instructions = parse_listing(text)
            instructions[0].raw = bytes([instructions[0].raw[0] ^ 1]) + instructions[0].raw[1:]
            return instructions
        with patch('hobbit.sema.disasm.parse_listing', side_effect=corrupt):
            self.unchanged(*pair_ending(BODY[:-2] + b'\xeb\xfa'))

    def test_return_immediate_before_lea_uses_actual_boundary(self):
        body = BODY[:-3] + b'\xc2\x08\x00'
        self.assertEqual(len(paired_sizes(*pair_ending(body))[2]), 1)
        self.unchanged(*pair_ending(BODY[:-5] + b'\xb8\x00\xc2\x08\x00'))


if __name__ == '__main__':
    unittest.main()
