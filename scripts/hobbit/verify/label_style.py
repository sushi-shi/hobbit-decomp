"""hobbit.verify.label_style - label-macro representation ratchet (fast).

Every label the pipeline reads is a MACRO with one canonical spelling
(AGENTS.md): 8-digit lowercase addresses, unpadded lowercase hex sizes, one
line per invocation (every label consumer is a single-line scan). Retired
comment-form label rows are FATAL; comment @markers come from the closed
vocabulary (docs/comment-markers.md); a volatile `_$E<n>` ordinal is
emission-order state, never a source label.

RVA binding: an RVA(..) annotates the declaration after it, so the next
declaration must be the labeled function's definition (reach `{` before
`;`). A bare declaration in between rebinds the address to that function
(a sweep once slid `inline b32 IsSelectable();` under SetState's label).
A macro-generated definition (BEGIN_MESSAGE_MAP, ...) is accepted.

MERGED (compgen_order.py): every RVA_COMPGEN invocation sits in RVA order
among its TU's other labeled lines - the intra-file monotonic-walk property.
(COMDAT copies are linker-pooled away from the TU's contiguous run, so this
is purely against each invocation's nearest labeled neighbours.)

    python3 -m hobbit.verify.label_style [--gate]
"""

from __future__ import annotations

import argparse
import re
from pathlib import Path

from hobbit.core.paths import REPO
from hobbit.verify.srcscan import (RVA_COMPGEN_RE, RVA_RE, blank_comments,
                                   source_files)

ADDR = r"0x[0-9a-f]{8}"
HEXN = r"(?:0x0|0x[1-9a-f][0-9a-f]*)"
MANGLED = r"[^\s,()]+"
VOLATILE_COMPGEN_RE = re.compile(r"\bRVA_COMPGEN\([^)]*,\s*_?\$E[0-9]+\s*\)")

CANON = {
    "RVA": rf"RVA\({ADDR}, {HEXN}\)",
    "DATA": rf"DATA\({ADDR}\)",
    "DATA_MESSAGE_MAP": rf"DATA_MESSAGE_MAP\({ADDR}, {ADDR}\)",
    "RVA_COMPGEN": rf"RVA_COMPGEN\({ADDR}, {HEXN}, {MANGLED}\)",
    # the owner charset mirrors the LIVE extraction regex (retail_labels.source
    # RVA_DYNINIT_RE): template-id owners (CActRegPool<CGrunt>::s_table) are
    # canonical - the frozen gate predated that convention
    "RVA_DYNINIT": rf"RVA_DYNINIT\({ADDR}, {HEXN}, [A-Za-z_][A-Za-z0-9_:<>]*\)",
    # expression-position macro: the value expression may spill past the line
    "DATA_COMPGEN": rf"DATA_COMPGEN\({ADDR},",
}
CANON_RE = {k: re.compile(v) for k, v in CANON.items()}
WRAPPABLE = {"RVA", "DATA"}   # StatementMacros clang-format arg-wraps past 100
FIND_RE = re.compile(r"\b(RVA_COMPGEN|RVA_DYNINIT|DATA_COMPGEN|DATA_MESSAGE_MAP|RVA|DATA)\s*\(")
COMMENT_ROW_RE = re.compile(r"@(?:rva|data)-symbol:\s*\S+\s+0x[0-9a-fA-F]+")
ALLOWED_MARKERS = {"stub", "early-stop", "identity-TODO", "confidence",
                   "source", "interleaver", "dead-code"}
MARKER_RE = re.compile(r"^\s*// ?@([A-Za-z][A-Za-z0-9_-]*)")


def scan(path: Path):
    raw = path.read_text(errors="replace")
    out = []
    for i, ln in enumerate(raw.splitlines(), 1):
        if COMMENT_ROW_RE.search(ln):
            out.append((i, "retired comment-form label row (use "
                           "RVA_COMPGEN/DATA)", ln.strip()[:90]))
        m = MARKER_RE.match(ln)
        if m and m.group(1) not in ALLOWED_MARKERS:
            out.append((i, f"@{m.group(1)} is not a blessed comment marker "
                           "(docs/comment-markers.md) - write plain prose",
                        ln.strip()[:90]))
    raw_lines = raw.splitlines()
    for i, ln in enumerate(blank_comments(raw).splitlines(), 1):
        if VOLATILE_COMPGEN_RE.search(ln):
            out.append((i, "volatile _$E<n> ordinal is evidence, not a source "
                           "label (pin the OWNER with RVA_DYNINIT)",
                        ln.strip()[:90]))
        for m in FIND_RE.finditer(ln):
            name = m.group(1)
            hit = CANON_RE[name].match(ln, m.start())
            if hit is None:
                out.append((i, f"{name}(..) off-canon (want {CANON[name]}; "
                               "one line, 8-digit addr, unpadded hex size)",
                            ln.strip()[:90]))
            elif name in WRAPPABLE and len(raw_lines[i - 1]) > 100:
                out.append((i, f"{name}(..) line exceeds 100 columns "
                               "(clang-format will wrap the invocation)",
                            ln.strip()[:90]))
    return out


RVA_CALL_RE = re.compile(r"\bRVA\s*\([^)]*\)")
MACRO_DEF_RE = re.compile(r"^\s*[A-Z][A-Z0-9_]*\s*\(", re.M)
LABEL_MACRO_RE = re.compile(r"^\s*(?:RVA|DATA|DATA_MESSAGE_MAP)\s*\(", re.M)


def rva_binding(path: Path):
    """RVA(..) labels whose next declaration is not a definition."""
    text = blank_comments(path.read_text(errors="replace"))
    out = []
    for m in RVA_CALL_RE.finditer(text):
        bol = text.rfind("\n", 0, m.start()) + 1
        if text[bol:m.start()].lstrip().startswith("#"):
            continue                          # inside a macro definition
        rest = text[m.end():]
        semi, brace = rest.find(";"), rest.find("{")
        if brace != -1 and (semi == -1 or brace < semi):
            continue
        head = rest[:semi if semi != -1 else len(rest)]
        if MACRO_DEF_RE.search(LABEL_MACRO_RE.sub("", head)):
            continue                          # macro-generated definition
        decl = " ".join(head.split())[:80]
        out.append((text.count("\n", 0, m.start()) + 1,
                    f"RVA(..) is not followed by its function definition "
                    f"(binds to the declaration `{decl};`)"))
    return out


def compgen_order(path: Path):
    """RVA_COMPGEN invocations out of RVA order among the TU's labeled lines."""
    seq = []
    for i, ln in enumerate(path.read_text(errors="replace").splitlines(), 1):
        m = RVA_RE.search(ln)
        if m:
            seq.append((i, int(m.group(1), 16), False))
            continue
        m = RVA_COMPGEN_RE.search(ln)
        if m:
            seq.append((i, int(m.group(1), 16), True))
    out = []
    for k, (ln, rva, is_cg) in enumerate(seq):
        if not is_cg:
            continue
        prev = seq[k - 1][1] if k > 0 else None
        nxt = seq[k + 1][1] if k + 1 < len(seq) else None
        if (prev is not None and rva < prev) or (nxt is not None and rva > nxt):
            out.append((ln, f"RVA_COMPGEN 0x{rva:06x} out of RVA order "
                            f"(between "
                            f"{'0x%06x' % prev if prev is not None else 'BOF'} "
                            f"and "
                            f"{'0x%06x' % nxt if nxt is not None else 'EOF'})"))
    return out


def violations() -> list[str]:
    out = []
    for path in source_files():
        r = path.relative_to(REPO)
        for line, why, text in scan(path):
            out.append(f"{r}:{line}: {why}\n    {text}")
        for line, why in rva_binding(path):
            out.append(f"{r}:{line}: {why}")
        if path.suffix == ".cpp":
            for line, why in compgen_order(path):
                out.append(f"{r}:{line}: {why}")
    return out


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="hobbit verify label-style",
                                 description=__doc__)
    ap.add_argument("--gate", action="store_true",
                    help="exit 1 on any off-canon label, marker, unbound RVA or "
                         "out-of-order RVA_COMPGEN")
    a = ap.parse_args(argv)
    viol = violations()
    for v in viol:
        print(v)
    if viol:
        print(f"label-style: {len(viol)} off-canon label/marker finding(s)")
        return 1 if a.gate else 0
    print("label-style: OK - canonical macros, blessed markers, "
          "RVA on definitions, RVA_COMPGEN in RVA order")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
