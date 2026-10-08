"""hobbit.walls.diagnose - classify one wall from the normalized pair.

    hobbit walls diagnose <rva|mangled|CClass::Member> [--asm]

The ladder (AGENTS.md): the FIRST divergence class decides the wall.

  referent   masked instruction bytes identical, only relocation TARGETS
             differ - an identity/aliasing question, not codegen. Lever:
             the reloc-sequence diff below; fix the claim, not the source.
  inline     the call-target multisets differ - a callee was expanded on
             one side and called on the other (or a call-set member is
             missing). Inspect `hobbit walls inline-model --gap` and
             docs/patterns/inline-budget-emits-ool-comdat.md; a call-set
             difference alone does not establish a budget deficit.
  cfg        call sets agree but branch/return counts differ - control-flow
             reconstruction (arm shape, tail merge, loop form). Check actual
             edges and source structure, not just branch totals.
  regalloc   same call multiset and branch/return counts, different bytes -
             provisional register/schedule candidate, NOT CFG equivalence.
             The ladder does not compare branch destinations or which calls
             each edge reaches. UpdateEntranceAnimation's misplaced SelectUnit
             passed this screen; audit semantic edges before steering. Lever:
             https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/relevations/cl5-callcrossing-ebx-first-by-use-schedule.md;
             a disposable A/B as described in
             docs/patterns/tu-state-probe-family-decides-reachability.md.

A `prior:` line follows the class when `config/codex_wall_reviews.tsv` already
holds a verdict for the row.  The two labels answer DIFFERENT questions and it
matters when they disagree: this tool names the FIRST divergence, so any branch
or return delta reads `cfg`, while a reviewer names the CAUSE they traced it to.
On 8 of the queue's 53 review-only rows that cause is register allocation with
the branch delta downstream of it - `CGruntPuddle`'s review says it outright
("the automatic CFG label is downstream codegen").  Without the line a matcher
reads "CFG - a structural reconstruction question" and hunts a source shape the
review already disproved.  The class above is still what the bytes say; the
`prior:` line only says who else has looked.

Inputs, read-only: the Model (locate the function), the compare out-dir's
NORMALIZED base/target objs (delink.coffx topology), tool.objdump for the
instruction skeleton. No retail-image reads here: the pair IS the evidence
objdiff scored.
"""

from __future__ import annotations

from collections import Counter
import re

from hobbit.core.paths import BUILD
from hobbit.delink.coffx import Obj

NORM = BUILD / "objdiff/compare-new"

_CALL = re.compile(r"\b(?:call)\s")
_RET = re.compile(r"\bret\b")
_JCC = re.compile(r"\bj(?:mp|e|ne|z|nz|a|ae|b|be|g|ge|l|le|s|ns|o|no|p|np)\b")


def _find_function(obj: Obj, name: str):
    """(payload, {offset: (target, addend)}, size) of `name`'s section slice.
    The slice runs from the symbol to the next defined symbol in its section."""
    for secnum in range(1, obj.nsec + 1):
        members = obj.section_members(secnum)
        value = next((v for v, n, _s in members if n == name), None)
        if value is None:
            continue
        payload = obj.section_payload(secnum)
        relocs = obj.typed_relocations(secnum)
        starts = sorted(v for v, _n, _s in members)
        end = next((s for s in starts if s > value), len(payload))
        rel = {off - value: tgt for off, tgt in relocs.items()
               if value <= off < end}
        from hobbit.walls.pairscan import function_body
        body = function_body(obj, secnum, value, end)
        rel = {off: target for off, target in rel.items() if off < len(body)}
        return body, rel, len(body)
    return None, None, 0


def _jump_table_bytes(rel: dict, own: str) -> set[int]:
    """Offsets covered by this function's own switch jump table.

    A DIR32 reloc whose referent is the function ITSELF is a table entry, not
    a call: the table is DATA embedded in .text, and objdump decodes it as
    instructions - some entry bytes decode as `call`, which used to inflate
    the call count and read as a false INLINE/CALL-SET wall on switch-heavy
    functions (CGrunt::StepCompassMove reported 24 vs 22 calls; both are 22)."""
    covered: set[int] = set()
    for off, (tgt, _addend) in rel.items():
        if tgt == own:
            covered.update(range(off, off + 4))
    return covered


def _skeleton(payload: bytes, rel: dict, vma: int = 0, data: set[int] = ()):
    """(masked bytes, calls-in-order, n_branches, n_returns, n_insns)."""
    from hobbit.tool import objdump
    masked = bytearray(payload)
    for off in rel:
        masked[off:off + 4] = b"\0\0\0\0"
    text = objdump.disassemble(payload, vma=vma)
    calls = branches = rets = insns = 0
    for line in text.splitlines():
        if ":\t" not in line:
            continue
        try:
            if int(line.split(":\t", 1)[0].strip(), 16) in data:
                continue
        except ValueError:
            pass
        insns += 1
        body = line.split("\t", 2)[-1]
        if _CALL.search(body):
            calls += 1
        elif _JCC.search(body):
            branches += 1
        elif _RET.search(body):
            rets += 1
    return bytes(masked), calls, branches, rets, insns, text


def _referents(rel: dict) -> list[str]:
    def spell(t):
        tgt, addend = t
        return f"{tgt}+0x{addend:x}" if addend else tgt
    return [spell(rel[o]) for o in sorted(rel)]


def ladder(bmask, tmask, bref, tref, bcalls, tcalls, skeleton) -> str:
    """The FIRST divergence class, as the docstring's ladder orders it.

    Split out so a sweep can ask the same question of many rows without
    re-implementing the order - a second copy of this would drift from the one
    that prints, and the two answers would disagree silently."""
    if bmask == tmask and bref != tref:
        return "referent"
    if bcalls != tcalls:
        return "inline"
    if skeleton[:2] != skeleton[2:]:
        return "cfg"
    if bmask != tmask:
        return "regalloc"
    return "none"


def named_functions() -> tuple:
    """The Model's NAMED function bindings.

    A SWEEP resolves this once and hands it to every `_locate` call.
    `hobbit.model.resolve` re-reads every census and claim channel each time
    (~0.33 s), so locating one row at a time paid that per row: `walls
    recheck` over 112 reviews spent 35 s of its 40 s here, which is the
    difference between a build gate and a manual sweep.  It is a parameter
    rather than a process-wide cache because a cache would outlive a mocked
    Model and answer from the previous test."""
    from hobbit.model import resolve
    return tuple(f for f in resolve().functions if f.name)


def _locate(token: str, named: tuple | None = None):
    """The claimed function a token names: a hex rva, the mangled name, or the
    readable `CClass::Member` spelling every other view accepts."""
    from hobbit.sema.index import short_name
    named = named_functions() if named is None else named
    if token.lower().startswith("0x"):
        try:
            rva = int(token, 16)
        except ValueError:
            return None, f"{token!r} is not a hex rva"
        b = next((f for f in named if f.rva == rva), None)
        if b is not None:
            return b, ""
        return None, (f"no CLAIMED function starts at {token} "
                      f"(`hobbit sema rva {token}` says what is there)")
    hits = [f for f in named if f.name == token] \
        or [f for f in named if short_name(f.name) == token]
    if len(hits) == 1:
        return hits[0], ""
    if not hits:
        return None, (f"no claimed function is named {token!r} (give the hex "
                      f"rva, the mangled name, or `CClass::Member`; "
                      f"`hobbit sema map find {token}` searches the Model)")
    where = ", ".join(f"0x{h.rva:06x} [{h.unit}]" for h in hits[:6])
    return None, f"{token!r} names {len(hits)} claimed functions: {where}"


def diagnose(token: str, show_asm: bool = False) -> int:
    b, why = _locate(token)
    if b is None:
        print(f"[diagnose] {why}")
        return 2
    base_p = NORM / "base" / f"{b.unit}.obj"
    tgt_p = next((p for p in (NORM / "target" / f"{b.unit}.c.obj",
                              NORM / "target" / f"{b.unit}.obj") if p.is_file()),
                 None)
    if not base_p.is_file() or tgt_p is None:
        print(f"[diagnose] normalized pair missing for {b.unit} - run "
              f"`hobbit compare` first")
        return 2

    from hobbit.tool import ToolError
    sides = {}
    for tag, path in (("base", base_p), ("target", tgt_p)):
        payload, rel, size = _find_function(Obj(path), b.name)
        if payload is None:
            print(f"[diagnose] {tag} obj does not define {b.name}")
            return 2
        try:
            table = _jump_table_bytes(rel, b.name)
            sides[tag] = (payload, rel, size,
                          *_skeleton(payload, rel, data=table))
        except ToolError as e:
            print(f"[diagnose] {e}")
            return 2

    (bp, brel, bsz, bmask, bcall, bbr, bret, bins, basm) = sides["base"]
    (tp, trel, tsz, tmask, tcall, tbr, tret, tins, tasm) = sides["target"]
    bref, tref = _referents(brel), _referents(trel)
    bcalls = Counter(n for n, _a in _call_targets(brel, basm, b.name))
    tcalls = Counter(n for n, _a in _call_targets(trel, tasm, b.name))

    print(f"[diagnose] {b.name}  [{b.unit}]  rva 0x{b.rva:06x}")
    print(f"  base:   {bsz:#x} B, {bins} insns, {bcall} calls, "
          f"{bbr} branches, {bret} rets, {len(brel)} relocs")
    print(f"  target: {tsz:#x} B, {tins} insns, {tcall} calls, "
          f"{tbr} branches, {tret} rets, {len(trel)} relocs")

    wall = ladder(bmask, tmask, bref, tref, bcalls, tcalls,
                  (bbr, bret, tbr, tret))
    if wall == "referent":
        print("  class: REFERENT - masked bytes identical; the relocation "
              "TARGETS differ:")
        for i, (x, y) in enumerate(zip(bref, tref)):
            if x != y:
                print(f"    reloc[{i}]: base {x}  !=  target {y}")
    elif wall == "inline":
        print("  class: INLINE/CALL-SET - the call-target multisets differ:")
        for n in sorted(bcalls.keys() | tcalls.keys()):
            bn, tn = bcalls[n], tcalls[n]
            if bn == tn:
                continue
            if bn and tn:
                print(f"    REPEATED-SITE DELTA: target {tn}, base {bn}: {n}")
            elif tn:
                print(f"    target calls, base expanded/lacks: {n}")
            else:
                print(f"    base calls, target expanded/lacks:  {n}")
        print(f"  lever: hobbit walls inline-model --gap 0x{b.rva:06x} "
              f"(defined symbol proves local emission; undefined/absent "
              f"symbols remain ambiguous)")
    elif wall == "cfg":
        print(f"  class: CFG - branch/return skeleton differs "
              f"(base {bbr}/{bret}, target {tbr}/{tret}); a structural "
              f"reconstruction question (arm shape, tail merge, loop form)")
    elif wall == "regalloc":
        first = next((i for i, (x, y) in enumerate(zip(bmask, tmask))
                      if x != y), min(len(bmask), len(tmask)))
        print(f"  class: REGALLOC/SCHEDULING - same call multiset and branch/return counts, "
              f"bytes first differ at +{first:#x}; instruction selection, "
              f"lifetime or allocation "
              f"(see docs/patterns/INDEX.md for evidence limits)")
        print("  provisional: branch destinations and side-effect reachability are not "
              "verified; audit semantic edges before compiler-state work")
    else:
        print("  class: NONE - the normalized pair is identical; the score "
              "gap is outside this function (pairing, data, or unit-level)")

    _prior_class(b.rva, wall)

    if wall in ("cfg", "regalloc", "inline"):
        _duplicate_tail_probe(basm, tasm, wall)

    if show_asm and wall != "none":
        print("  --- base ---")
        print("\n".join("  " + ln for ln in basm.splitlines()[:60]))
        print("  --- target ---")
        print("\n".join("  " + ln for ln in tasm.splitlines()[:60]))
    return 0


def prior_class_note(row, fresh: bool, wall: str) -> str | None:
    """The line to print when a review already classified this row.

    The two labels answer DIFFERENT questions.  This function names the FIRST
    divergence, so any branch or return delta reads CFG; a reviewer names the
    CAUSE they traced it to, and on 8 of the queue's review-only rows that
    cause is register allocation with the branch delta downstream of it.  A
    matcher who sees only "CFG - a structural reconstruction question" goes
    hunting for a source shape those reviews already disproved.
    """
    if row is None:
        return None
    state = "current" if fresh else "STALE, body edited since"
    if row["wall_class"] == wall:
        return (f"  prior: review agrees ({row['status']}/{row['wall_class']}, "
                f"{state}) - hobbit walls priors")
    return (f"  prior: a review classified this {row['status']}/"
            f"{row['wall_class']}, not {wall} ({state}). This tool names the "
            f"FIRST divergence; the review names the CAUSE - read it before "
            f"treating the label above as the lead: hobbit walls priors")


def _prior_class(rva: int, wall: str) -> None:
    try:
        from hobbit.walls.reviews import current as _cur, load as _load
        row, fresh = _load().get(rva), rva in set(_cur())
    except Exception:
        return
    note = prior_class_note(row, fresh, wall)
    if note:
        print(note)


def _insn_text(asm: str) -> list[str]:
    """One normalized mnemonic+operand string per instruction, in order.

    Intra-function branch displacements survive here on purpose: two arms that
    jump to DIFFERENT continuations must not read as identical runs."""
    out = []
    for line in asm.splitlines():
        if ":\t" not in line:
            continue
        out.append(" ".join(line.split("\t", 2)[-1].split()))
    return out


def _repeat_runs(insns: list[str], minlen: int = 4):
    """Maximal repeated instruction runs, each classified SUFFIX vs PREFIX.

    Only a converging SUFFIX is foldable at all (the unconditional suffix
    cross-jump is /Os-gated and off in our /O2 build; what merges under /O2 is
    value-based factoring - wall-reasons-layout.md), so the distinction
    decides whether a duplicated run could ever have been merged:
      suffix - every copy leaves via the same terminator (ret, or a jmp to one
               target); the copies converge, so the pass COULD merge them.
      prefix - the copies diverge after the run; no merge pass would fold them,
               and the duplication is a CFG reconstruction difference.
    Returns [(length, n_copies, kind, last_insn)] sorted longest-first."""
    index: dict[str, list[int]] = {}
    for i, t in enumerate(insns):
        index.setdefault(t, []).append(i)
    # text of a maximal run -> the distinct start positions it occurs at
    found: dict[tuple[str, ...], set[int]] = {}
    for positions in index.values():
        if len(positions) < 2:
            continue
        for a in range(len(positions)):
            for b in range(a + 1, len(positions)):
                i, j = positions[a], positions[b]
                # extend backwards to the run's true start, then forwards
                s = 0
                while (i - s - 1 >= 0 and j - s - 1 > i - s - 1
                       and insns[i - s - 1] == insns[j - s - 1]):
                    s += 1
                i0, j0 = i - s, j - s
                n = 0
                while (j0 + n < len(insns)
                       and insns[i0 + n] == insns[j0 + n]):
                    n += 1
                if n < minlen:
                    continue
                found.setdefault(tuple(insns[i0:i0 + n]),
                                 set()).update((i0, j0))
    out = []
    for text, starts in found.items():
        n = len(text)
        last = text[-1]
        terminal = bool(_RET.search(last)) or last.startswith("jmp")
        # a run is a foldable SUFFIX when every copy leaves the same way:
        # a terminator, or an identical following instruction
        after = {insns[s + n] if s + n < len(insns) else None for s in starts}
        kind = "suffix" if (terminal or len(after) == 1) else "prefix"
        out.append((n, len(starts), kind, last))
    out.sort(key=lambda r: (-r[0], -r[1]))
    return out


MIN_RUN = 10


def _duplicate_tail_probe(basm: str, tasm: str, wall: str) -> None:
    """List the LONG repeated runs on each side, classified suffix vs prefix.

    Short runs repeat by chance in any large body (a 4-insn window recurs
    hundreds of times), so only runs of MIN_RUN+ instructions are evidence.
    The routing question is per-RUN, not per-count: a long SUFFIX duplicated
    on one side only is a blocked cross-jump; long PREFIXES that diverge are
    never foldable and mean the CFG differs. This prints the evidence and
    names the rule; it does not pretend to adjudicate the function."""
    b = _repeat_runs(_insn_text(basm), MIN_RUN)
    t = _repeat_runs(_insn_text(tasm), MIN_RUN)
    if not b and not t:
        return

    def show(tag, runs):
        if not runs:
            print(f"  {tag}: no repeated run >= {MIN_RUN} insns")
            return
        for n, c, kind, last in runs[:4]:
            print(f"  {tag}: {c}x {n}-insn {kind} run, ends `{last}`")

    show("base  ", b)
    show("target", t)
    bs = [r for r in b if r[2] == "suffix"]
    ts = [r for r in t if r[2] == "suffix"]
    if [(n, c, k) for n, c, k, _l in b] == [(n, c, k) for n, c, k, _l in t]:
        print("    -> SYMMETRIC: both sides duplicate the same runs, so the "
              "duplication is retail's own shape (a no-IL tail or a per-arm "
              "scope it really had), not a defect - do not chase it.")
    elif bs and not ts:
        print("    -> only BASE duplicates a long converging SUFFIX. The "
              "VC5 donor associated this with /Os-gated cross-jumps; Hobbit "
              "compiler behavior needs calibration. Inspect factoring and look "
              "for a join at the suffix head, a per-arm destructible local, "
              "or arm VALUES that differ where retail's agree. "
              "https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/relevations/wall-reasons-layout.md")
    elif ts and not bs:
        print("    -> only TARGET duplicates a long suffix: retail's arms "
              "carried something ours factored away (a per-arm scope is the "
              "usual one).")
    elif (b or t) and not bs and not ts:
        if wall == "inline":
            print("    -> the long repeats are PREFIXES that diverge, but the "
                  "call-set already differs: this can be site-positioned "
                  "inline-budget residue. Compare ordered call sites before "
                  "inferring a CFG reconstruction defect.")
        else:
            print("    -> the long repeats are PREFIXES that diverge; no "
                  "merge pass folds those, so a duplication difference here "
                  "is a CFG reconstruction question, not a placement coin.")


def _call_targets(rel: dict, asm: str, own: str | None = None) -> list[tuple[str, int]]:
    """Call referents, including a delinked relocation-free self call.

    A relative call whose destination is the start of the sliced function is
    unambiguously recursive even when the target delinker did not attach a
    relocation.  Without this control ImportDirectoryTree was falsely classified as
    INLINE/CALL-SET although both sides contain the same 21 calls and the linked
    image resolves the site back to ImportDirectoryTree.
    """
    call_offs = set()
    unrelocated_self = []
    for line in asm.splitlines():
        if ":\t" not in line:
            continue
        addr, rest = line.split(":\t", 1)
        body = rest.split("\t")[-1]
        if _CALL.search(body):
            try:
                call_off = int(addr.strip(), 16)
                call_offs.add(call_off)
                if own is not None and re.search(r"\bcall\s+0x0\b", body):
                    unrelocated_self.append(call_off)
            except ValueError:
                pass
    out = []
    for off in sorted(rel):
        # a REL32 call operand starts 1 byte after the opcode
        if (off - 1) in call_offs or (off - 2) in call_offs:
            out.append(rel[off])
    relocated_calls = {off - 1 for off in rel} | {off - 2 for off in rel}
    out.extend((own, 0) for off in unrelocated_self if off not in relocated_calls)
    return out


def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(
        prog="hobbit walls diagnose", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("token", help="hex rva, mangled name, or CClass::Member")
    ap.add_argument("--asm", action="store_true",
                    help="print both sides' disassembly (first 60 lines)")
    a = ap.parse_args(argv)
    return diagnose(a.token, a.asm)


if __name__ == "__main__":
    raise SystemExit(main())
