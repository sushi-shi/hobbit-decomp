"""hobbit.walls.semdiff - OPERAND-LEVEL adjudication of a normalized pair.

`walls diagnose` CLASSIFIES a wall (referent -> inline/call-set -> cfg ->
regalloc). It does not ADJUDICATE it: its call-set and branch-skeleton view
cannot see a member read at the wrong displacement, a swapped store source,
a dropped `fild`, or a mask that lost a bit. Those are the live-bug classes,
and they are all multiset questions over the two sides' operands.

This module answers them. It reads the same evidence objdiff scored (the
normalized pair under build/objdiff/compare-new) and compares, as multisets:

    fp      FP opcode counts        cl NEVER adds or drops a conversion to
                                    schedule - any delta is semantic
    disp    member displacements    [reg+N] with reg != esp: the fields the
                                    function actually reads and writes
    store   store targets           the displacement side of `mov [reg+N],x`
    imm     immediates              constants, masks, magic numbers
    mnem    mnemonic counts         the coarse shape, printed last

The EXCLUSIVE section is printed FIRST and is the whole point: a key one side
uses and the other never touches. `b3/t4` on a shared key is almost always
scheduling; `b0/t2` is a constant, offset or conversion that exists on one
side only, and that is where the four bugs of 2026-08-20 were found.

Two more sections cover what a value-level multiset structurally cannot see,
because objdiff MASKS relocated operands:

    referent sequence   the ORDERED list of relocation targets. A swapped
                        pair of string keys, or a global bound to the wrong
                        symbol, is invisible in the scored bytes and obvious
                        here.
    cmd/key pairing     the ordered (pushed small immediate -> string
                        referent) pairs: "which widget got which command id
                        and which sprite key", which neither the value
                        multiset nor the referent order decides alone.

FALSE-POSITIVE TAXONOMY. Every one of these was observed producing a
difference on a pair whose semantics are identical; the filters that can be
applied mechanically are applied, the rest are for the reader:

  * register mirrors        `cmp a,b/jge` == `cmp b,a/jle`; `cdq` == `sar 31`
                            for the signed-modulo sign mask. FILTERED: no.
  * register-width mirror   `and al,0xe0` == `and ecx,0xffffffe0` (cl uses
                            the 2-byte AL form when the value lands in EAX).
                            FILTERED: yes - an 8/16-bit AND/OR/XOR immediate
                            is keyed as the 32-bit operation it performs
                            (AND fills the untouched bytes with ones, OR/XOR
                            with zeros; an AH-class register shifts by 8).
  * EH state stores         `mov [esp+N],state` into the /GX unwind-state
                            slot numbers the destructible objects, not a
                            constant of the function. FILTERED: yes (the
                            `walls eh-frame` slot rule).
  * cross-jump merge degree one side shares a cleanup/call site the other
                            duplicates. Changes call-SITE counts, never the
                            set of paths that reach the call. FILTERED: no.
  * lea-folded displacement retail `lea eax,[base+0x150]` then `[eax+0x24]`
                            vs ours `[base+0x174]` - the same address, the
                            array base folded into the member offset.
                            FILTERED: no (compare the SUM).
  * operand width           a byte store read back as a dword when the upper
                            bytes are known zero. FILTERED: no.
  * rep-stos first-dword    a `memset` whose first element is stored
                            separately from the `rep stos` tail.
  * RMW split/fold          `add [m],1` vs `mov r,[m]; inc r; mov [m],r`
                            changes the disp count for one member.
  * FP-stack housekeeping   `fmulp` vs `fmul st,st(1)` + a later `fstp` -
                            the POP discipline, not a conversion. fld/fstp
                            deltas are noise; fild/fistp deltas are not.
  * jump-table data         a function's own index/target table decodes as
                            junk instructions with huge displacements.
                            FILTERED: yes - decoding stops at the first table
                            the function's own `jmp [r*4+T]` / `mov dl,[r+I]`
                            addresses (cl emits every table after the code),
                            and those self-relocated operands are not keys.
  * byte-continuation       the second line of a long instruction carries
                            bytes and no mnemonic. FILTERED: yes.
  * frame-size immediates   `sub esp,N` / `add esp,N` / `ret N` are the
                            local-slot count and the callee cleanup, never
                            semantics. FILTERED: yes.
  * one-past-end referent   `sym+size` aliases the NEXT symbol's name, so
                            one side reads `foo+0x400` and the other
                            `bar+1` for the same address. FILTERED: no.

FALSE-NEGATIVE LIMIT: identical operand multisets and ordered referents do
not prove identical side-effect reachability. Moving SelectUnit across the
current-player guard in UpdateEntranceAnimation preserved both. Audit branch
destinations and per-edge call traces.

    hobbit walls semdiff <rva|name> [--top N] [--all]
    hobbit walls semsweep <tsv> [--first N] [--last N] [--fp-only]

semsweep screens a worklist (the `walls inventory` TSV shape: unit, name,
rva, pct, size, class, detail) and prints ONE line per row unless the row has
an exclusive key or an FP delta - the shape that made a 379-row lane
tractable in one pass.
"""

from __future__ import annotations

import argparse
import re
from collections import Counter
from difflib import SequenceMatcher

from hobbit.core.paths import BUILD
from hobbit.delink.coffx import Obj
from hobbit.tool import objdump
from hobbit.walls.diagnose import _find_function, _jump_table_bytes, _locate

NORM = BUILD / "objdiff/compare-new"

#: `[reg+N]` / `[reg+idx*s+N]`, esp deliberately excluded - a stack slot is a
#: frame-layout accident, a member displacement is the class model.
MEM = re.compile(r"\[(e[a-d]x|e[sd]i|ebx|ecx|ebp)(?:\+e[a-z]{2}\*\d)?"
                 r"([+-]0x[0-9a-f]+)?\]")
#: the same operand with EBP as its base, and the two spellings that point EBP
#: into the STACK. cl 5.0 at /O2 usually spends EBP as a general register, and
#: then `[ebp+N]` IS a member displacement - but it gives a handful of
#: functions an ebp frame (`push ebp; mov ebp,esp` in the prologue: there
#: every `[ebp-N]` is a stack slot), and it also parks a pointer to a local in
#: EBP mid-function (`lea ebp,[esp+0xc]` to pass a local's address, `mov
#: ebp,esp` over an array at the stack top). The second is a stack pointer
#: only until the next write to EBP; a function-wide mask there hides every
#: `this`-in-ebp member access elsewhere in the body.
EBP_MEM = re.compile(r"\[ebp(?:\+e[a-z]{2}(?:\*\d)?)?([+-]0x[0-9a-f]+)?\]")
EBP_FRAME = re.compile(
    r"^(?:mov\s+ebp,esp|lea\s+ebp,\[esp(?:[+-]0x[0-9a-f]+)?\])$")
EBP_WRITE = re.compile(r"^(?:mov|movzx|movsx|lea|xor|or|and|add|sub|adc|sbb"
                       r"|inc|dec|neg|not|shl|shr|sar|imul|pop|xchg)\s+ebp\b")
#: the prologue window a frame pointer is established in
FRAME_WINDOW = 6
#: an 8/16-bit AND/OR/XOR with an immediate: the register-width mirror
NARROW_LOGIC = re.compile(
    r"^(and|or|xor)\s+(?:([a-d][lhx]|[sd]i|[sb]p)|"
    r"(BYTE|WORD) PTR [^,]+),(0x[0-9a-f]+)$")
#: a function's own jump table (`jmp [r*4+T]`) and byte index table
#: (`mov dl,[r+I]`); both operands carry a relocation naming the function
TABLE_OPERAND = re.compile(
    r"^(?:jmp\s+DWORD PTR \[e[a-z]{2}\*4\+|"
    r"mov\s+[a-d]l,BYTE PTR \[e[a-z]{2}\+)(0x[0-9a-f]+)\]$")
IMM = re.compile(r"(?<![\[+])\b0x[0-9a-f]+\b")
FP = re.compile(r"^(fild|fistp?|fld|fstp?|fmul|fdiv|fadd|fsub|fcom|fchs|fabs"
                r"|frndint|fxch|fsqrt|fnstsw)\w*")
BYTES_ONLY = re.compile(r"^(?:[0-9a-f]{2} ?)+$")
FRAME = re.compile(r"^(sub|add)\s+esp,")
NOT_A_VALUE = re.compile(r"^(j\w+|call|loop|ret)$")


def _prologue_frame(lines) -> bool:
    """`push ebp` then `mov ebp,esp` / `lea ebp,[esp+N]` before any transfer."""
    head = lines[:FRAME_WINDOW]
    for i, ln in enumerate(head[:-1]):
        if ln.asm.startswith(("j", "call", "ret")):
            return False
        if ln.asm == "push ebp" and EBP_FRAME.match(head[i + 1].asm):
            return True
    return False


def ebp_is_frame(*sides) -> bool:
    """Whether EBP is the FRAME pointer, in which case every `[ebp+-N]`
    operand is a stack slot and not a member displacement.

    Only a prologue establishment counts; a mid-body `lea ebp,[esp+N]` is a
    pointer to one local and `features` masks it over its own live range.

    Take BOTH sides. cl gives one side an ebp frame and not the other
    (`CGrunt::StepBrickLayerBehavior` is ours, retail's ebp is a general
    register there), and masking only the side that has one is a mask that
    manufactures a difference. If either side's ebp is a frame pointer, the
    operand is not comparable on that row and both sides mask.
    """
    return any(_prologue_frame(lines) for lines in sides)


def ebp_stack_lines(lines) -> set[int]:
    """Indexes of the lines where EBP points into the stack because a mid-body
    `lea ebp,[esp+N]` / `mov ebp,esp` set it: from that line up to the next
    write to EBP (straight-line order - the pointer's live range)."""
    out: set[int] = set()
    live = False
    for i, ln in enumerate(lines):
        if EBP_FRAME.match(ln.asm):
            live = True
            continue
        if EBP_WRITE.match(ln.asm):
            live = False
            continue
        if live:
            out.add(i)
    return out


class Line:
    """One decoded instruction: address, asm text, and the relocation
    referent whose operand it carries (masked to 0 in the scored bytes)."""

    __slots__ = ("addr", "asm", "ref")

    def __init__(self, addr: int, asm: str, ref: str | None):
        self.addr, self.asm, self.ref = addr, asm, ref


def table_start(lines: list[Line], self_name: str) -> int | None:
    """The offset of the function's first switch table, or None.

    cl 5.0 emits every jump table and byte index table AFTER the function's
    code, so the lowest table a real `jmp [r*4+T]` / `mov dl,[r+I]` addresses
    is where the instructions end. The byte index table carries no relocation
    at all, so the self-relocated DIR32 entries cannot bound it. An operand
    must point FORWARD: a junk line decoded inside a table cannot move the
    bound below the real one.
    """
    starts = []
    for ln in lines:
        if ln.ref != self_name:
            continue
        m = TABLE_OPERAND.match(ln.asm)
        if m and int(m.group(1), 16) > ln.addr:
            starts.append(int(m.group(1), 16))
    return min(starts, default=None)


def _decode(body: bytes, rel: dict, self_name: str | None = None) -> list[Line]:
    """Disassemble one function window and attach each relocation to the
    instruction whose bytes contain it.

    Given the function's own name, two normalizations that every consumer
    needs and none of them can do afterwards:

      * the function's own jump/index TABLES are DATA embedded in .text, and
        objdump decodes them as instructions.  Decoding stops at the first
        table (`table_start`), and a line starting inside a self-relocated
        DIR32 entry is dropped (`_jump_table_bytes`, the rule `walls
        diagnose` applies).  A real self-transfer is never dropped by this:
        its relocation sits at `addr+1`, so the instruction's own address is
        not covered.
      * a relocation that names the function ITSELF on a real instruction is
        a self-transfer - a recursive `call`, a tail `jmp`, or the table
        operand of a switch - and the delinked target resolves a transfer
        inside its own section with NO relocation at all.  The two sides then
        disagree about a name that only means "here".  The line keeps the
        self-referent (so `features` can drop the table operand as a key) and
        `referent_runs` skips it.
    """
    table = _jump_table_bytes(rel, self_name) if self_name else ()
    out: list[Line] = []
    for line in objdump.disassemble(body, vma=0).splitlines():
        if ":\t" not in line:
            continue
        head, rest = line.split(":\t", 1)
        try:
            addr = int(head.strip(), 16)
        except ValueError:
            continue
        if addr in table:
            continue
        parts = rest.split("\t")
        asm = " ".join((parts[-1] if len(parts) > 1 else parts[0]).split())
        nbytes = len((parts[0] if len(parts) > 1 else "").split())
        ref = None
        for off, (target, _add) in rel.items():
            if addr <= off < addr + max(nbytes, 1):
                ref = re.sub(r"\+0x[0-9a-f]+$", "", target)
                break
        out.append(Line(addr, asm, ref))
    if self_name:
        cut = table_start(out, self_name)
        if cut is not None:
            out = [ln for ln in out if ln.addr < cut]
    return out


def pair_lines(token: str):
    """(binding, base lines, target lines) for one claimed function."""
    b, why = _locate(token)
    if b is None:
        raise SystemExit(f"[semdiff] {why}")
    base = Obj(NORM / "base" / f"{b.unit}.obj")
    tgt_path = NORM / "target" / f"{b.unit}.c.obj"
    if not tgt_path.exists():
        tgt_path = NORM / "target" / f"{b.unit}.obj"
    tgt = Obj(tgt_path)
    bb, brel, _bs = _find_function(base, b.name)
    tb, trel, _ts = _find_function(tgt, b.name)
    return b, _decode(bb, brel, b.name), _decode(tb, trel, b.name)


def eh_state_lines(lines: list[Line]) -> set[int]:
    """Addresses of the stores into the /GX unwind-state slot (the `walls
    eh-frame` slot rule); their immediates number objects, not values."""
    from hobbit.walls import eh_frame
    insns = [(ln.addr, *(ln.asm.split(None, 1) + [""])[:2]) for ln in lines]
    if not eh_frame.has_eh(insns):
        return set()
    _slot, states = eh_frame.eh_states(insns)
    return {off for off, _state in states}


def value_immediates(asm: str) -> list[str]:
    """The immediates of one instruction as the 32-bit values they act as.

    An 8/16-bit AND/OR/XOR is the register-width mirror of the 32-bit
    operation on the whole register: `and al,0xe0` IS `and eax,0xffffffe0`,
    `or ah,0xc` IS `or eax,0xc00`."""
    m = NARROW_LOGIC.match(asm)
    if not m:
        return IMM.findall(asm)
    op, reg, ptr, imm = m.groups()
    width = 16 if (ptr == "WORD" or (reg and len(reg) == 2
                                     and reg[1] in "xip")) else 8
    shift = 8 if reg in ("ah", "bh", "ch", "dh") else 0
    mask = ((1 << width) - 1) << shift
    v = (int(imm, 16) << shift) & mask
    if op == "and":
        v |= 0xFFFFFFFF & ~mask
    return IMM.findall(asm[:m.start(4)]) + [f"0x{v:x}"]


def features(lines: list[Line], self_name: str = "",
             ebp_frame: bool | None = None) -> dict[str, Counter]:
    """The five multisets, with the mechanical filters applied.

    Dropped here: the operands of the function's own table references
    (self-referent lines keep only their mnemonic), byte-continuation lines,
    the frame-size and callee-cleanup immediates, the /GX unwind-state
    stores' immediates, and the `[ebp+-N]` operands where EBP points into the
    stack - function-wide in the handful of functions cl gives an ebp frame,
    over its live range where EBP holds a local's address. Pass `ebp_frame`
    from BOTH sides (`ebp_is_frame(base, target)`); reading it off one side
    masks asymmetrically. Narrow AND/OR/XOR immediates are keyed as the
    32-bit operation (`value_immediates`).
    """
    if ebp_frame is None:
        ebp_frame = ebp_is_frame(lines)
    stack_ebp = ebp_stack_lines(lines)
    eh_states = eh_state_lines(lines)
    disp, imm, mnem, fp, store = (Counter() for _ in range(5))
    for i, ln in enumerate(lines):
        asm = ln.asm
        if not asm or BYTES_ONLY.match(asm):
            continue
        if FRAME.match(asm):
            continue
        if ebp_frame or i in stack_ebp:
            asm = EBP_MEM.sub("[esp]", asm)
        op = asm.split()[0]
        mnem[op] += 1
        if self_name and ln.ref == self_name:
            continue
        m = FP.match(asm)
        if m:
            fp[m.group(1)] += 1
        for _reg, d in MEM.findall(asm):
            disp[d or "+0x0"] += 1
        if op == "mov" and "," in asm:
            dst = asm.split(",", 1)[0]
            for _reg, d in MEM.findall(dst):
                store[d or "+0x0"] += 1
        if not NOT_A_VALUE.match(op) and ln.addr not in eh_states:
            for v in value_immediates(asm):
                if int(v, 16) > 3:
                    imm[v] += 1
    return dict(fp=fp, disp=disp, store=store, imm=imm, mnem=mnem)


def exclusive(fb: dict, ft: dict) -> list[tuple[str, str, int, int]]:
    """Keys ONE side uses and the other never touches - the semantic signal."""
    out = []
    for kind in ("fp", "disp", "store", "imm"):
        cb, ct = fb[kind], ft[kind]
        for key in sorted(set(cb) | set(ct)):
            if (cb[key] == 0) != (ct[key] == 0):
                out.append((kind, key, cb[key], ct[key]))
    return out


def referent_runs(lines: list[Line], self_name: str = "") -> list[str]:
    """The ordered referent sequence, consecutive duplicates collapsed; the
    function's own name means "here" and only one side spells it."""
    out: list[str] = []
    for ln in lines:
        if ln.ref and ln.ref != self_name and (not out or out[-1] != ln.ref):
            out.append(ln.ref)
    return out


def cmd_key_pairs(lines: list[Line]) -> list[tuple[str, str]]:
    """Ordered (pushed small immediate -> next string referent) pairs."""
    out, pending = [], None
    for ln in lines:
        m = re.fullmatch(r"push (0x[0-9a-f]+)", ln.asm)
        if m and 0x20 <= int(m.group(1), 16) <= 0x200:
            pending = m.group(1)
        if ln.ref and ln.ref.startswith("??_C@") and pending:
            out.append((pending, ln.ref[:52]))
            pending = None
    return out


def _seq_report(label: str, rb: list, rt: list, ctx: int = 6) -> int:
    sm = SequenceMatcher(None, rb, rt, autojunk=False)
    ops = [o for o in sm.get_opcodes() if o[0] != "equal"]
    print(f"== {label}: base {len(rb)} target {len(rt)}, "
          f"{len(ops)} divergence(s)")
    for op, i1, i2, j1, j2 in ops:
        print(f"   {op} base[{i1}:{i2}] target[{j1}:{j2}]")
        for x in rb[i1:i2][:ctx]:
            print(f"      B {x}")
        for x in rt[j1:j2][:ctx]:
            print(f"      T {x}")
    return len(ops)


def adjudicate(token: str, top: int = 30, show_all: bool = False) -> int:
    b, lb, lt = pair_lines(token)
    ebp = ebp_is_frame(lb, lt)
    fb, ft = features(lb, b.name, ebp), features(lt, b.name, ebp)
    exc = exclusive(fb, ft)

    print(f"{b.name}  0x{b.rva:06x}  [{b.unit}]")
    print(f"== EXCLUSIVE (one side only) -- the semantic signal: {len(exc)}")
    for kind, key, u, v in exc:
        print(f"   {kind:6} {key:>14}  base {u:3d}  target {v:3d}")
    if not exc:
        print("   (none)")

    for kind in ("fp", "disp", "store", "imm", "mnem"):
        cb, ct = fb[kind], ft[kind]
        keys = sorted(set(cb) | set(ct), key=lambda x: -abs(cb[x] - ct[x]))
        diffs = [(x, cb[x], ct[x]) for x in keys if cb[x] != ct[x]]
        if not diffs and not show_all:
            print(f"== {kind}: identical "
                  f"(base {sum(cb.values())} target {sum(ct.values())})")
            continue
        print(f"== {kind}: {len(diffs)} differing key(s)")
        for x, u, v in diffs[:top]:
            print(f"   {x:>14}  base {u:3d}  target {v:3d}")

    _seq_report("referent sequence", referent_runs(lb, b.name),
                referent_runs(lt, b.name))
    _seq_report("cmd/key pairing", cmd_key_pairs(lb), cmd_key_pairs(lt))
    return 0


def screen(token: str) -> tuple[list, list]:
    """(exclusive keys, FP deltas) - the two sweep-visible signals."""
    b, lb, lt = pair_lines(token)
    ebp = ebp_is_frame(lb, lt)
    fb, ft = features(lb, b.name, ebp), features(lt, b.name, ebp)
    fpd = [(k, fb["fp"][k], ft["fp"][k])
           for k in sorted(set(fb["fp"]) | set(ft["fp"]))
           if fb["fp"][k] != ft["fp"][k]]
    return exclusive(fb, ft), fpd


def sweep(rows, first: int, last: int, fp_only: bool) -> int:
    flagged = 0
    for n, row in enumerate(rows, 1):
        if n < first or (last and n > last):
            continue
        rva, name = row["rva"], row["name"]
        try:
            exc, fpd = screen(rva)
        except SystemExit as exc_err:
            print(f"### {n} {rva} {name} -- {exc_err}")
            continue
        if fp_only:
            exc = []
        if not exc and not fpd:
            print(f"### {n} {rva} {row['pct']} {row['cls']} {name} -- CLEAN")
            continue
        flagged += 1
        print(f"### {n} {rva} {row['pct']} {row['cls']} {name}")
        for kind, key, u, v in exc[:8]:
            print(f"   {kind:6} {key:>14}  base {u:3d}  target {v:3d}")
        for k, u, v in fpd:
            print(f"   FP!    {k:>14}  base {u:3d}  target {v:3d}")
    print(f"\nflagged {flagged} row(s)")
    return 0


def _read_worklist(path: str) -> list[dict]:
    rows = []
    with open(path) as fh:
        for raw in fh:
            f = raw.rstrip("\n").split("\t")
            if len(f) < 3:
                continue
            rows.append({"unit": f[0], "name": f[1], "rva": f[2],
                         "pct": f[3] if len(f) > 3 else "",
                         "size": f[4] if len(f) > 4 else "",
                         "cls": f[5] if len(f) > 5 else ""})
    return rows


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="hobbit walls semdiff",
                                 description=__doc__.split("\n\n")[0])
    ap.add_argument("token", help="hex rva, mangled name, or CClass::Member")
    ap.add_argument("--top", type=int, default=30)
    ap.add_argument("--all", action="store_true",
                    help="print identical sections too")
    args = ap.parse_args(argv)
    return adjudicate(args.token, args.top, args.all)


def sweep_main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="hobbit walls semsweep",
                                 description="screen a worklist TSV")
    ap.add_argument("tsv", help="worklist: unit, name, rva, pct, size, class")
    ap.add_argument("--first", type=int, default=1)
    ap.add_argument("--last", type=int, default=0)
    ap.add_argument("--fp-only", action="store_true",
                    help="report only FP-opcode deltas")
    args = ap.parse_args(argv)
    return sweep(_read_worklist(args.tsv), args.first, args.last,
                 args.fp_only)


if __name__ == "__main__":
    raise SystemExit(main())
