"""Carve Hobbit VC6 /GX exception funclets and their owner-derived tables.

Ports Gruntz's retail registration-stub/unwind-map ownership traversal. The
PC 12.00.9044 compiler object proves a 28-byte FuncInfo header, preceded by
its 8-byte-per-state unwind map. Actual PC map pointers independently prove
that adjacency. These are target settings, not Gruntz's VC5 layout.

Only decoded PUSH instructions in claimed bodies lead to registration stubs.
A stub must load a valid FuncInfo and branch; the table points to its cleanup
code. Unknown record shapes remain withheld, never padded to a guessed size.
The stable symbols describe owner/role, not volatile compiler ordinals.
"""

from __future__ import annotations

import struct
import re
from dataclasses import dataclass
from pathlib import Path

FUNCINFO_MAGIC = 0x19930520
PUSH_IMM32 = 0x68
MOV_EAX_IMM32 = 0xB8
JMP_REL32 = 0xE9
STUB_SIZE = 10          # `b8 imm32` + `e9 rel32`

EHREG_PREFIX = "__ehreg$"
EHUNWIND_PREFIX = "__ehunwind$"
EHFUNCINFO_PREFIX = "__ehfuncinfo$"
EHUNWINDMAP_PREFIX = "__ehunwindmap$"

#: Hobbit VC6 12.00.9044 emits seven words. Actual xstring .xdata$x
#: COMDATs place their unwind map before this 28-byte record; the PC retains
#: that order. Gruntz's VC5 32-byte record/forward map is not this ABI.
FUNCINFO_SIZE = 28
#: `_s_UnwindMapEntry` = {int toState; void (*action)();}
UNWIND_ENTRY_SIZE = 8


def registration_symbol(owner: str) -> str:
    return EHREG_PREFIX + owner


def unwind_symbol(owner: str, index: int) -> str:
    return f"{EHUNWIND_PREFIX}{owner}${index}"


def funcinfo_symbol(owner: str) -> str:
    return EHFUNCINFO_PREFIX + owner


def unwindmap_symbol(owner: str) -> str:
    return EHUNWINDMAP_PREFIX + owner


def is_band_symbol(name: str) -> bool:
    """True for the two CODE names this module carves."""
    return name.startswith(EHREG_PREFIX) or name.startswith(EHUNWIND_PREFIX)


def is_band_data_symbol(name: str) -> bool:
    """True for the two `.xdata$x` names this module carves."""
    return name.startswith(EHFUNCINFO_PREFIX) or name.startswith(EHUNWINDMAP_PREFIX)


@dataclass(frozen=True)
class Group:
    owner_rva: int
    owner: str
    unit: str
    funclets: tuple[int, ...]   # unwind funclet starts, ascending (== state order)
    stub: int                   # the pushed registration stub
    funcinfo: int = 0           # the `_s_FuncInfo` the stub loads into eax
    states: int = 0             # its maxState == the unwind map's entry count
    packed: bool = False        # map ends exactly at the FuncInfo header
    simple: bool = False        # ... and it has no try-block / ip-to-state map
    unwind_map: int = 0          # decoded pointer, not assumed record + size

    @property
    def start(self) -> int:
        return self.funclets[0] if self.funclets else self.stub

    @property
    def end(self) -> int:
        return self.stub + STUB_SIZE

    @property
    def funcinfo_size(self) -> int:
        """The blob's proven extent, or 0 when the record does not prove one."""
        if not (self.packed and self.simple) or self.states <= 0:
            return 0
        return FUNCINFO_SIZE + UNWIND_ENTRY_SIZE * self.states


class _Image:
    def __init__(self, path: Path):
        self.data = Path(path).read_bytes()
        pe = struct.unpack_from("<I", self.data, 0x3C)[0]
        count = struct.unpack_from("<H", self.data, pe + 6)[0]
        optional = struct.unpack_from("<H", self.data, pe + 20)[0]
        self.image_base = struct.unpack_from("<I", self.data, pe + 24 + 28)[0]
        self.sections = []
        for index in range(count):
            base = pe + 24 + optional + index * 40
            name = self.data[base:base + 8].rstrip(b"\0").decode("latin-1")
            vsize, vaddr, raw_size, raw_ptr = struct.unpack_from(
                "<IIII", self.data, base + 8)
            self.sections.append((name, vaddr, max(vsize, raw_size), raw_ptr, raw_size))
        text = next(s for s in self.sections if s[0] == ".text")
        self.text_lo, self.text_hi = text[1], text[1] + text[2]

    def read(self, rva: int, size: int) -> bytes | None:
        for _name, vaddr, vsize, raw_ptr, raw_size in self.sections:
            if vaddr <= rva and rva + size <= vaddr + min(vsize, raw_size):
                offset = raw_ptr + (rva - vaddr)
                if offset + size > len(self.data):
                    return None
                return self.data[offset:offset + size]
        return None


def _funclets(image: _Image, funcinfo_rva: int) -> tuple[set[int], int, bool, bool] | None:
    header = image.read(funcinfo_rva, 28)
    if header is None:
        return None
    magic, states, unwind_map, tries, try_map, n_ip, p_ip = struct.unpack_from(
        "<IiIiIiI", header, 0)
    if magic != FUNCINFO_MAGIC or states < 0 or tries < 0:
        return None
    # The record itself states where its unwind map is; when the complete map
    # immediately precedes the record, its span is independently proven.
    # The 28-byte record ABI is corroborated by actual VC6 candidate objects.
    packed = unwind_map - image.image_base + states * UNWIND_ENTRY_SIZE == funcinfo_rva
    simple = tries == 0 and try_map == 0 and n_ip == 0 and p_ip == 0
    out: set[int] = set()
    if states:
        entries = image.read(unwind_map - image.image_base, 8 * states)
        if entries is None:
            return None
        for index in range(states):
            _to_state, action = struct.unpack_from("<iI", entries, index * 8)
            if not -1 <= _to_state < index:
                return None
            if action:
                if not image.text_lo <= action - image.image_base < image.text_hi:
                    return None
                out.add(action - image.image_base)
    for index in range(tries):
        block = image.read(try_map - image.image_base + index * 20, 20)
        if block is None:
            return None
        _low, _high, _catch_high, catches, handler_array = struct.unpack_from(
            "<iiiiI", block, 0)
        table = image.read(handler_array - image.image_base, 16 * catches)
        if table is None:
            return None
        for slot in range(catches):
            _adjectives, _type, _disp, handler = struct.unpack_from(
                "<IIiI", table, slot * 16)
            out.add(handler - image.image_base)
    return out, states, packed, simple


def groups(exe: Path, names_map: dict[int, tuple]) -> list[Group]:
    """Every EH funclet group owned by a function `names_map` attributes to a unit.

    ``names_map`` is pdb_synth's ``{rva: (name, unit, size)}`` overlay (longer
    tuples are accepted; only the first three fields are read). Returned groups
    are sorted by start RVA and never overlap.
    """
    image = _Image(exe)
    found: dict[int, Group] = {}
    for rva in sorted(names_map):
        entry = names_map[rva]
        name, unit, size = entry[0], entry[1], entry[2]
        if len(entry) > 3 and entry[3] != "func":
            continue
        if size <= 0 or not unit:
            continue
        body = image.read(rva, size)
        if body is None:
            continue
        # Decode from the admitted function boundary, as Gruntz's static
        # destructor oracle does. Bytes resembling PUSH inside an immediate
        # are not evidence of an exception-registration instruction.
        from hobbit.tool.objdump import disassemble
        offsets = []
        for line in disassemble(body).splitlines():
            match = re.match(r"^\s*([0-9a-f]+):\s+(?:[0-9a-f]{2}\s+)+\s*\t(push\s+.*)$", line)
            if match:
                offsets.append(int(match[1], 16))
        for offset in offsets:
            if offset + 5 > len(body) or body[offset] != PUSH_IMM32:
                continue
            stub = struct.unpack_from("<I", body, offset + 1)[0] - image.image_base
            if not image.text_lo <= stub < image.text_hi:
                continue
            code = image.read(stub, STUB_SIZE)
            if not (code and code[0] == MOV_EAX_IMM32 and code[5] == JMP_REL32):
                continue
            funcinfo = struct.unpack_from("<I", code, 1)[0] - image.image_base
            parsed = _funclets(image, funcinfo)
            if parsed is None:
                continue
            addresses, states, packed, simple = parsed
            if addresses and max(addresses) >= stub:
                # A funclet at or past the registration stub would make the
                # group non-contiguous; refuse to guess an extent for it.
                continue
            found[stub] = Group(owner_rva=rva, owner=name, unit=unit,
                                funclets=tuple(sorted(addresses)), stub=stub,
                                funcinfo=funcinfo, states=states,
                                packed=packed, simple=simple,
                                unwind_map=struct.unpack_from('<I', image.read(funcinfo + 8, 4))[0] - image.image_base)
    ordered = sorted(found.values(), key=lambda g: g.start)
    for previous, current in zip(ordered, ordered[1:]):
        if current.start < previous.end:
            raise ValueError(
                f"EH band groups overlap: 0x{previous.start:08x}..0x{previous.end:08x} "
                f"({previous.owner}) and 0x{current.start:08x} ({current.owner})")
    return ordered


def records(band: list[Group]) -> list[tuple[int, str, str, int]]:
    """``(rva, symbol, unit, size)`` rows for pdb_synth's names_map overlay."""
    out = []
    for group in band:
        bounds = list(group.funclets) + [group.stub]
        for index, start in enumerate(group.funclets):
            out.append((start, unwind_symbol(group.owner, index), group.unit,
                        bounds[index + 1] - start))
        out.append((group.stub, registration_symbol(group.owner), group.unit,
                    STUB_SIZE))
    return out


def data_records(band: list[Group]) -> list[tuple[int, str, str, int]]:
    """``(rva, symbol, unit, size)`` rows for the `.xdata$x` blob of each group.

    Two definitions per group, both with an extent the FuncInfo record itself
    proves (see `Group.funcinfo_size`); a group whose record does not prove one
    is silently absent rather than guessed at.
    """
    out = []
    for group in band:
        extent = group.funcinfo_size
        if not extent:
            continue
        out.append((group.funcinfo, funcinfo_symbol(group.owner), group.unit,
                    FUNCINFO_SIZE))
        out.append((group.unwind_map, unwindmap_symbol(group.owner),
                    group.unit, extent - FUNCINFO_SIZE))
    return out
