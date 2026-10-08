"""hobbit.walls.switchmap - the case-to-arm mapping of every switch jump table.

A switch whose case table is permuted scores within a few percent of exact:
the arms are the same instructions, and only the 4-byte table entries (and
the byte index table in front of them) disagree.  objdiff masks the entries
as relocations, and every multiset screen (`walls semdiff`) sees the same
constants on both sides - so a wrong case-to-arm mapping is invisible to all
of them.  It is a real behaviour bug every time: the multiplayer checksum's
tool permutation and the entrance idle-facing snap were both this.

This module decodes both sides' tables and follows each case:

    jmp DWORD PTR [r*4+T]         the jump table (self-relocated entries)
    mov dl,BYTE PTR [r+I]         an optional byte index table in front of it
    cmp r,N / ja default          the bound: cases 0..N (N may sit in a
                                  register loaded just before)
    add r,-K | sub r,K | dec r    the bias: case value = index + K

and for every case value walks the arm it reaches - through unconditional
jumps, to the first conditional branch or ret (capped by `--depth`) -
collecting the SET of constants, displacements, referents, callees and the
exit kind.  Registers, `[esp+N]` slots and /GX state stores are ignored;
`lea r,[r-0x20]` / `sub r,0x20` key as the value they add.

Arm contents are not compared case by case: a wall in a shared tail changes
every case at once.  A case is a hit when its MAPPING differs - the cases
sharing its arm differ between base (ours) and target (retail), or another
arm on the other side matches it strictly better (see `compare`).  The
retail table's VA is printed so the fix can cite it.

    hobbit walls switchmap <rva|name> [--depth N] [--verbose]
    hobbit walls switchmap --all [--depth N]
    hobbit walls switchmap --tsv WORKLIST [--depth N]

`--all` screens every sub-100 function in the compare report; `--tsv` takes
the `walls inventory` / semsweep worklist shape (unit, name, rva, ...).
A hit is a lead, not a verdict: cl can hoist a constant out of one arm into
the dispatch block (`mov ebp,1` before the `jmp`), which regroups the arms
without changing behaviour.  Read both arms (`hobbit sema disasm <rva>`)
before calling it a bug.
"""

from __future__ import annotations

import argparse
import re
import struct
from collections import Counter
from dataclasses import dataclass, field

from hobbit.walls.semdiff import (BYTES_ONLY, Line, _decode, eh_state_lines,
                                  value_immediates)

from hobbit.core.pe import image
#: arm instructions collected per case; the walk also stops at the first
#: conditional branch or ret, so this only caps a long straight-line arm
DEPTH = 64
#: how far back from the `jmp` the bound / index load may sit
LOOKBACK = 14

TABLE_JMP = re.compile(r"^jmp DWORD PTR \[(e[a-z]{2})\*4\+(0x[0-9a-f]+)\]$")
INDEX_LOAD = re.compile(
    r"^mov [a-d]l,BYTE PTR \[(e[a-z]{2})\+(0x[0-9a-f]+)\]$")
BOUND = re.compile(r"^cmp (e[a-z]{2}),(0x[0-9a-f]+|e[a-z]{2})$")
DIRECT_JMP = re.compile(r"^jmp (0x[0-9a-f]+)$")
STACK_SLOT = re.compile(r"\[esp(?:\+0x[0-9a-f]+)?\]")
DISP = re.compile(r"([+-]0x[0-9a-f]+)\]")
EXIT_RET, EXIT_JCC = "<ret>", "<jcc>"
REG = re.compile(r"\b(?:e[a-d]x|e[sd]i|e[sb]p|[a-d][lhx]|[sd]i|[sb]p)\b")


def _s32(v: int) -> int:
    return v - (1 << 32) if v & 0x80000000 else v


@dataclass
class Case:
    value: int
    arm: int                       # arm offset in the function
    keys: Counter = field(default_factory=Counter)
    text: list[str] = field(default_factory=list)

    @property
    def keyset(self) -> frozenset:
        """The keys as a SET: an RMW split (`add [m],k` vs `mov r,[m]; add
        r,k; mov [m],r`) or a duplicated load changes counts, not values."""
        return frozenset(self.keys)

    @property
    def informative(self) -> bool:
        """Whether the arm reaches anything but its exit: `test r,r; je` has
        nothing to compare, and cl hoists a load into or out of such an arm
        freely."""
        return bool(self.keyset - {EXIT_RET, EXIT_JCC})


@dataclass
class Switch:
    jmp: int                       # offset of the `jmp [r*4+T]`
    table: int                     # jump table offset
    index: int | None              # byte index table offset
    bound: int | None              # highest index (cases 0..bound)
    bias: int                      # case value = index + bias
    cases: list[Case]


def _bias(lines: list[Line], at: int, reg: str) -> int:
    """The constant cl subtracted from the switch value before the bound
    check, read from the few lines before the `cmp`."""
    for k in range(at - 1, max(-1, at - 6), -1):
        a = lines[k].asm
        if a.startswith(("j", "ret", "call")):
            break
        m = re.match(rf"^(add|sub) {reg},(0x[0-9a-f]+)$", a)
        if m:
            v = _s32(int(m.group(2), 16))
            return -v if m.group(1) == "add" else v
        if a == f"dec {reg}":
            return 1
        if a == f"inc {reg}":
            return -1
        m = re.match(rf"^lea {reg},\[e[a-z]{{2}}([+-]0x[0-9a-f]+)\]$", a)
        if m:
            return -int(m.group(1), 16)
        if a.split(None, 1)[-1].startswith(reg + ","):
            break                  # another write to the register: no bias
    return 0


def _bound(lines: list[Line], at: int, jmp: int, reg: str) -> int | None:
    """The bound of a `cmp reg,N` / `cmp reg,r2` (r2 loaded with N just
    before) that guards the table: a `ja`, and no other transfer, between it
    and the table `jmp` (cl schedules unrelated stores in between)."""
    m = BOUND.match(lines[at].asm)
    if not m or m.group(1) != reg:
        return None
    between = [ln.asm for ln in lines[at + 1:jmp] if ln.asm.startswith("j")]
    if len(between) != 1 or not between[0].startswith("ja "):
        return None
    if m.group(2).startswith("0x"):
        return int(m.group(2), 16)
    for k in range(at - 1, max(-1, at - 6), -1):
        mv = re.match(rf"^mov {m.group(2)},(0x[0-9a-f]+)$", lines[k].asm)
        if mv:
            return int(mv.group(1), 16)
    return None


def _keys(ln: Line, self_name: str) -> tuple[Counter, str]:
    """What one arm instruction contributes: constants, displacements,
    referents, callees - registers and stack slots dropped."""
    a = STACK_SLOT.sub("[esp]", ln.asm)
    ref = ln.ref if ln.ref and ln.ref != self_name else None
    keys: Counter = Counter()
    if a.startswith("call"):
        keys["call " + (ref or REG.sub("R", a.split(None, 1)[-1]))] += 1
    elif a.startswith("lea ") and "[esp]" not in a:
        # `lea r,[r-0x20]` is `add r,0xffffffe0`: key the value it adds
        for d in DISP.findall(a):
            keys[f"0x{int(d, 16) & 0xFFFFFFFF:x}"] += 1
    elif re.match(r"^sub e[a-z]{2},0x[0-9a-f]+$", a):
        keys[f"0x{-int(a.rsplit(',', 1)[1], 16) & 0xFFFFFFFF:x}"] += 1
    else:
        for v in value_immediates(a):
            keys[v] += 1
        for d in DISP.findall(a):
            keys[d] += 1
        if ref:
            keys[f"<{ref}>"] += 1
    text = REG.sub("R", a) + (f" <{ref}>" if ref else "")
    return keys, text


def _walk(lines, byaddr, start: int, depth: int, self_name: str,
          skip: set[int] = frozenset()):
    keys: Counter = Counter()
    text: list[str] = []
    j = byaddr.get(start)
    if j is None:
        return keys, [f"?{start:#x}"]
    seen: set[int] = set()
    steps = 0
    while j is not None and j < len(lines) and steps < depth and j not in seen:
        seen.add(j)
        a = lines[j].asm
        if not a or BYTES_ONLY.match(a):
            j += 1
            continue
        m = DIRECT_JMP.match(a)
        if m:
            j = byaddr.get(int(m.group(1), 16))
            continue
        if a.startswith(("j", "ret")):
            # how the arm leaves - returning vs branching on - is part of
            # it; the branch SENSE is not (block layout inverts je/jne)
            text.append(a.split()[0])
            keys[EXIT_RET if a.startswith("ret") else EXIT_JCC] += 1
            break
        k, t = _keys(lines[j], self_name)
        if lines[j].addr not in skip:
            keys.update(k)
        text.append(t)
        steps += 1
        j += 1
    return keys, text


def switches(body: bytes, lines: list[Line], self_name: str = "",
             depth: int = DEPTH) -> list[Switch]:
    """Every jump-table switch in one decoded function, with each case's
    reached keys.  `lines` is `semdiff._decode(body, rel, self_name)`."""
    byaddr = {ln.addr: i for i, ln in enumerate(lines)}
    eh_states = eh_state_lines(lines)
    out: list[Switch] = []
    for i, ln in enumerate(lines):
        m = TABLE_JMP.match(ln.asm)
        if not m:
            continue
        table = int(m.group(2), 16)
        if not ln.addr < table < len(body):
            continue
        index = bound = None
        bias = 0
        reg = m.group(1)
        for k in range(i - 1, max(-1, i - LOOKBACK), -1):
            a = lines[k].asm
            if a.startswith("ret") or DIRECT_JMP.match(a):
                break
            mi = INDEX_LOAD.match(a)
            if mi and index is None:
                v = int(mi.group(2), 16)
                if lines[k].addr < v < len(body):
                    index, reg = v, mi.group(1)
            got = _bound(lines, k, i, reg)
            if got is not None:
                bound = got
                bias = _bias(lines, k, reg)
                break
        if bound is None and index is None:
            # no visible bound: the entries up to the body's end or the
            # first entry that is not a code offset
            n = 0
            while (table + 4 * n + 4 <= len(body)
                   and struct.unpack_from("<I", body, table + 4 * n)[0]
                   < table):
                n += 1
            bound = n - 1 if n else None
        cases: list[Case] = []
        for v in range((bound if bound is not None else -1) + 1):
            if index is not None:
                if index + v >= len(body):
                    break
                idx = body[index + v]
            else:
                idx = v
            if table + 4 * idx + 4 > len(body):
                break
            arm = struct.unpack_from("<I", body, table + 4 * idx)[0]
            keys, text = _walk(lines, byaddr, arm, depth, self_name,
                               eh_states)
            cases.append(Case(v + bias, arm, keys, text))
        out.append(Switch(ln.addr, table, index, bound, bias, cases))
    return out


@dataclass
class Hit:
    switch: int
    value: int | None
    base: Case | None
    target: Case | None
    note: str = ""


def _dist(a: frozenset, b: frozenset) -> int:
    return len(a ^ b)


def _groups(cases: dict[int, Case]) -> dict[int, frozenset[int]]:
    """case value -> the case values whose arms reach the same key set; an
    uninformative arm groups with nothing."""
    by: dict[frozenset, set[int]] = {}
    for v, c in cases.items():
        if c.informative:
            by.setdefault(c.keyset, set()).add(v)
    return {v: frozenset(by[c.keyset]) if c.informative else frozenset({v})
            for v, c in cases.items()}


def compare(sb: list[Switch], st: list[Switch]) -> list[Hit]:
    """The cases whose MAPPING differs, switch by switch in order.

    Arm contents are not compared directly: a wall in a shared tail, or cl
    scheduling across the window, changes the keys of every case at once and
    says nothing about which arm a case reaches.  A mapping difference is
    RELATIONAL, and either of two signals flags case v:

      * grouping  - the set of cases whose arms reach the same keys as v's
                    differs between the sides (a case moved to, or out of,
                    another case's arm - the default included);
      * closer    - another arm on the other side matches v's arm strictly
                    better than v's own counterpart does (a permutation).

    An arm that reaches nothing but its exit (`test r,r; je`) takes part in
    neither: it carries no evidence, and cl hoists loads across it freely.
    """
    hits: list[Hit] = []
    if len(sb) != len(st):
        hits.append(Hit(-1, None, None, None,
                        f"switch count base {len(sb)} target {len(st)}"))
    for n, (x, y) in enumerate(zip(sb, st)):
        bx = {c.value: c for c in x.cases}
        by = {c.value: c for c in y.cases}
        for v in sorted(set(bx) ^ set(by)):
            hits.append(Hit(n, v, bx.get(v), by.get(v),
                            "case on one side only"))
        common = sorted(set(bx) & set(by))
        gb = _groups({v: bx[v] for v in common})
        gt = _groups({v: by[v] for v in common})
        for v in common:
            cb, ct = bx[v], by[v]
            if cb.keyset == ct.keyset and gb[v] == gt[v]:
                continue
            if gb[v] != gt[v]:
                shared_b = sorted(gb[v] - {v})
                shared_t = sorted(gt[v] - {v})
                hits.append(Hit(n, v, cb, ct,
                                f"arm shared with base {shared_b or '-'} "
                                f"target {shared_t or '-'}"))
                continue
            if not (cb.informative and ct.informative):
                continue
            own = _dist(cb.keyset, ct.keyset)
            better = sorted({w for w in common if w != v and (
                (by[w].informative
                 and _dist(cb.keyset, by[w].keyset) < own)
                or (bx[w].informative
                    and _dist(bx[w].keyset, ct.keyset) < own))})
            if better:
                hits.append(Hit(n, v, cb, ct,
                                f"closer to the arm of case(s) {better}"))
    return hits


def pair(token: str, depth: int = DEPTH):
    """(binding, base switches, target switches) for one claimed function."""
    from hobbit.delink.coffx import Obj
    from hobbit.walls.diagnose import _find_function, _locate
    from hobbit.walls.semdiff import NORM
    b, why = _locate(token)
    if b is None:
        raise SystemExit(f"[switchmap] {why}")
    sides = []
    for path in (NORM / "base" / f"{b.unit}.obj",
                 next((p for p in (NORM / "target" / f"{b.unit}.c.obj",
                                   NORM / "target" / f"{b.unit}.obj")
                       if p.is_file()), None)):
        if path is None or not path.is_file():
            raise SystemExit(f"[switchmap] normalized pair missing for "
                             f"{b.unit} - run `hobbit build` first")
        body, rel, _n = _find_function(Obj(path), b.name)
        if body is None:
            raise SystemExit(f"[switchmap] {b.name} not in {path.name}")
        sides.append(switches(body, _decode(body, rel, b.name), b.name,
                              depth))
    return b, sides[0], sides[1]


def _fmt_keys(c: Case | None, width: int = 160) -> str:
    if c is None:
        return "(absent)"
    text = " ; ".join(c.text) or "(nothing reached)"
    return text if len(text) <= width else text[:width - 3] + "..."


def report(token: str, depth: int, verbose: bool, quiet: bool) -> int:
    """Print one function's switches; the number of hits."""
    b, sb, st = pair(token, depth)
    hits = compare(sb, st)
    if quiet and not hits:
        return 0
    print(f"{b.name}  0x{b.rva:06x}  [{b.unit}]  "
          f"{len(sb)}/{len(st)} switch(es), {len(hits)} differing case(s)")
    for n, (x, y) in enumerate(zip(sb, st)):
        mine = [h for h in hits if h.switch == n]
        if not mine and not verbose:
            continue
        lo = min((c.value for c in y.cases), default=0)
        hi = max((c.value for c in y.cases), default=-1)
        print(f"  switch {n}: jmp base {x.jmp:#x} target {y.jmp:#x}; "
              f"retail table 0x{image().image_base + b.rva + y.table:08x}"
              + (f" index 0x{image().image_base + b.rva + y.index:08x}"
                 if y.index is not None else "")
              + f"; cases {lo}..{hi}")
        if verbose:
            by = {c.value: c for c in y.cases}
            for c in x.cases:
                flag = "!!" if any(h.value == c.value for h in mine) else "  "
                print(f"   {flag} {c.value:4d}  B {_fmt_keys(c)}")
                if flag == "!!":
                    print(f"            T {_fmt_keys(by.get(c.value))}")
            continue
        for h in mine:
            print(f"   case {h.value:4d}: {h.note}")
            print(f"      B {_fmt_keys(h.base)}")
            print(f"      T {_fmt_keys(h.target)}")
    for h in hits:
        if h.switch < 0:
            print(f"  {h.note}")
    return len(hits)


def sweep(tokens: list[str], depth: int) -> int:
    fns = hit_fns = cases = errors = 0
    for tok in tokens:
        try:
            n = report(tok, depth, False, True)
        except SystemExit as err:
            errors += 1
            print(f"  skip {tok}: {err}")
            continue
        fns += 1
        if n:
            hit_fns += 1
            cases += n
    print(f"\n[switchmap] {fns} function(s) screened, {hit_fns} with a "
          f"mapping difference ({cases} case(s)), {errors} skipped")
    return 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="hobbit walls switchmap",
                                 description=__doc__.split("\n\n")[0])
    g = ap.add_mutually_exclusive_group(required=True)
    g.add_argument("token", nargs="?",
                   help="hex rva, mangled name, or CClass::Member")
    g.add_argument("--all", action="store_true",
                   help="every sub-100 function in the compare report")
    g.add_argument("--tsv", help="worklist: unit, name, rva, ...")
    ap.add_argument("--depth", type=int, default=DEPTH,
                    help="arm instructions compared per case (default "
                         f"{DEPTH})")
    ap.add_argument("--verbose", action="store_true",
                    help="print every case of one function, not only hits")
    a = ap.parse_args(argv)
    if a.token:
        report(a.token, a.depth, a.verbose, False)
        return 0
    if a.all:
        from hobbit.verify.scores import is_eh_band
        from hobbit.walls.inventory import build
        tokens = [r["rva"] for r in build() if r["rva"]
                  and not is_eh_band(r["symbol"])]
    else:
        from hobbit.walls.semdiff import _read_worklist
        tokens = [r["rva"] for r in _read_worklist(a.tsv)]
    return sweep(tokens, a.depth)


if __name__ == "__main__":
    raise SystemExit(main())
