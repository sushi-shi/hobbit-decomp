from __future__ import annotations

import struct
import unittest

from hobbit.walls.semdiff import (Line, _decode, ebp_is_frame, exclusive,
                                  features)

ME = "?Fn@C@@QAEHXZ"


def _lines(asms):
    return [Line(i * 4, a, None) for i, a in enumerate(asms)]


def _keys(base, target):
    ebp = ebp_is_frame(base, target)
    return {(k, key) for k, key, _u, _v in exclusive(
        features(base, ME, ebp), features(target, ME, ebp))}


def _switch_body(pad: int):
    """xor edx,edx / mov dl,[eax+I] / jmp [edx*4+T] / two `mov eax,K; ret`
    arms, `pad` nops, the jump table, then a byte index table - the same
    function with its tables at a different offset."""
    code = bytes.fromhex("33d2" "8a90" "00000000" "ff2495" "00000000"
                         "b80a000000c3" "b814000000c3") + b"\x90" * pad
    table = len(code)
    index = table + 8
    body = bytearray(code + struct.pack("<II", 0x0F, 0x15)
                     + bytes([1, 0, 0, 1, 1, 0]))
    struct.pack_into("<I", body, 4, index)
    struct.pack_into("<I", body, 0xB, table)
    rel = {4: (ME, 6), 0xB: (ME, 6), table: (ME, 6), table + 4: (ME, 6)}
    return bytes(body), rel, table


class JumpTableControls(unittest.TestCase):
    def test_tables_at_another_offset_screen_clean(self):
        """The table operands and the reloc-less index table are not keys:
        before the fix `[eax+0x24]` vs `[eax+0x28]` read as a member swap."""
        (ba, ra, ta), (bb, rb, tb) = _switch_body(1), _switch_body(5)
        la, lb = _decode(ba, ra, ME), _decode(bb, rb, ME)
        self.assertTrue(all(ln.addr < ta for ln in la))
        self.assertTrue(all(ln.addr < tb for ln in lb))
        self.assertEqual(_keys(la, lb), set())


class WidthMirrorControls(unittest.TestCase):
    def test_narrow_logic_equals_its_32_bit_mirror(self):
        self.assertEqual(_keys(_lines(["and al,0xe0", "or ah,0xc"]),
                               _lines(["and ecx,0xffffffe0", "or eax,0xc00"])),
                         set())

    def test_a_different_mask_is_still_flagged(self):
        """`and al,0x1f` keeps the upper bytes; `and eax,0x1f` clears them."""
        self.assertEqual(_keys(_lines(["and al,0x1f"]),
                               _lines(["and eax,0x1f"])),
                         {("imm", "0xffffff1f"), ("imm", "0x1f")})


PROLOGUE = ["push 0xffffffff", "push 0x0", "mov eax,fs:0x0", "push eax",
            "mov DWORD PTR fs:0x0,esp", "sub esp,0x8", "push esi"]


class EhStateControls(unittest.TestCase):
    def test_state_numbering_is_not_a_constant(self):
        self.assertEqual(
            _keys(_lines(PROLOGUE + ["mov DWORD PTR [esp+0x14],0x5"]),
                  _lines(PROLOGUE + ["mov DWORD PTR [esp+0x14],0x6"])),
            set())

    def test_a_local_store_keeps_its_constant(self):
        self.assertEqual(
            _keys(_lines(PROLOGUE + ["mov DWORD PTR [esp+0x4],0x5"]),
                  _lines(PROLOGUE + ["mov DWORD PTR [esp+0x4],0x6"])),
            {("imm", "0x5"), ("imm", "0x6")})


class EbpControls(unittest.TestCase):
    def test_a_local_pointer_masks_only_its_live_range(self):
        base = _lines(["push ebp", "lea ebp,[esp+0x10]",
                       "mov eax,DWORD PTR [ebp+0x4]", "push ebp"])
        target = _lines(["push ebp", "lea ebp,[esp+0x14]",
                         "mov eax,DWORD PTR [ebp+0x8]", "push ebp"])
        self.assertEqual(_keys(base, target), set())

    def test_this_in_ebp_is_not_masked_by_a_later_local_pointer(self):
        base = _lines(["push ebp", "mov ebp,ecx",
                       "mov eax,DWORD PTR [ebp+0x218]",
                       "lea ebp,[esp+0x10]", "push ebp"])
        target = _lines(["push ebp", "mov ebp,ecx",
                         "mov eax,DWORD PTR [ebp+0x21c]"])
        self.assertEqual(_keys(base, target),
                         {("disp", "+0x218"), ("disp", "+0x21c")})

    def test_a_prologue_frame_masks_its_slots(self):
        self.assertEqual(
            _keys(_lines(["push ebp", "mov ebp,esp",
                          "mov eax,DWORD PTR [ebp-0x8]"]),
                  _lines(["push ebp", "mov ebp,esp",
                          "mov eax,DWORD PTR [ebp-0xc]"])),
            set())


if __name__ == "__main__":
    unittest.main()
