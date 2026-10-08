from __future__ import annotations

import struct
import unittest

from hobbit.walls.semdiff import Line
from hobbit.walls.switchmap import compare, switches

ME = "?Fn@C@@QAEHXZ"
TABLE = 0x44


def _fn(reg: str, base: str, arms: dict[int, int], table: list[int],
        index: list[int] | None = None):
    """A switch on (v-1) over three `mov reg,K; jmp tail` arms, a
    `mov [base+0x10],reg; ret` tail and a `xor; ret` default.  `arms` maps
    each constant to its arm address, `table` lists the arm addresses in
    table order, `index` is an optional byte index table."""
    lines = [Line(0x00, "dec eax", None),
             Line(0x01, "cmp eax,0x2", None),
             Line(0x04, "ja 0x40", None)]
    if index is not None:
        lines += [Line(0x06, "xor edx,edx", None),
                  Line(0x08, f"mov dl,BYTE PTR [eax+{TABLE + 12:#x}]", ME),
                  Line(0x0E, f"jmp DWORD PTR [edx*4+{TABLE:#x}]", ME)]
    else:
        lines += [Line(0x06, f"jmp DWORD PTR [eax*4+{TABLE:#x}]", ME)]
    for k, at in sorted(arms.items(), key=lambda kv: kv[1]):
        lines += [Line(at, f"mov {reg},{k:#x}", None),
                  Line(at + 5, "jmp 0x30", None)]
    lines += [Line(0x30, f"mov DWORD PTR [{base}+0x10],{reg}", None),
              Line(0x33, "ret", None),
              Line(0x40, f"xor {reg},{reg}", None),
              Line(0x42, "ret", None)]
    body = bytearray(TABLE) + struct.pack(f"<{len(table)}I", *table)
    if index is not None:
        body += bytes(index)
    return switches(bytes(body), lines, ME)


ARMS = {0xA: 0x10, 0x14: 0x18, 0x1E: 0x20}


class SwitchMapControls(unittest.TestCase):
    def test_a_permuted_table_is_flagged(self):
        base = _fn("ecx", "esi", ARMS, [0x10, 0x18, 0x20])
        target = _fn("ecx", "esi", ARMS, [0x18, 0x10, 0x20])
        self.assertEqual([c.value for c in base[0].cases], [1, 2, 3])
        self.assertEqual({h.value for h in compare(base, target)}, {1, 2})

    def test_a_permuted_index_table_is_flagged(self):
        table = [0x10, 0x18, 0x20]
        base = _fn("ecx", "esi", ARMS, table, index=[0, 1, 2])
        target = _fn("ecx", "esi", ARMS, table, index=[0, 2, 2])
        self.assertEqual({h.value for h in compare(base, target)}, {2, 3})

    def test_registers_and_arm_layout_alone_screen_clean(self):
        """Same mapping; other registers, and the arms laid out in reverse
        order so every table entry differs."""
        base = _fn("ecx", "esi", ARMS, [0x10, 0x18, 0x20])
        moved = {0xA: 0x20, 0x14: 0x18, 0x1E: 0x10}
        target = _fn("edx", "edi", moved, [0x20, 0x18, 0x10])
        self.assertEqual(compare(base, target), [])


if __name__ == "__main__":
    unittest.main()
