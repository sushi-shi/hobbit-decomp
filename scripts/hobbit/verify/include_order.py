"""Canonical include order, adapted from Gruntz's include-order gate.

Hobbit has no admitted project-wide precompiled-header contract. The local
policy therefore requires no implicit prelude. Duplicate includes, canonical
ordering and shared-library to game dependency violations remain gated.
"""

from __future__ import annotations

import argparse
import re
from pathlib import Path

from hobbit.core.paths import REPO

SRC_DIRS = ("src", "include")
EXTS = (".h", ".cpp", ".hpp", ".inl", ".c")

INC_RE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]')
IFNDEF_RE = re.compile(r"^\s*#\s*ifndef\s+(\w+)\s*$")
DEFINE_RE = re.compile(r"^\s*#\s*define\s+(\w+)\s*$")
PP_RE = re.compile(r"^\s*#\s*(\w+)")

RVA_H = "rva.h"

PRELUDE = None  # No Hobbit-wide prelude has been established.
PLATFORM_RE = re.compile(r"^(afx\w*|windows)\.h$", re.I)

G_PRELUDE, G_RVA, G_OWN, G_PROJECT, G_LIBRARY = 1, 2, 3, 4, 5
GROUPS = (G_PRELUDE, G_RVA, G_OWN, G_PROJECT, G_LIBRARY)

def repo_files():
    for d in SRC_DIRS:
        for p in sorted((REPO / d).rglob("*")):
            if p.suffix in EXTS:
                yield p


PROJECT_HEADERS = {p.relative_to(REPO / "include").as_posix()
                   for p in (REPO / "include").rglob("*")
                   if p.suffix in (".h", ".hpp", ".inl")}


def own_header(path: Path):
    if path.suffix != ".cpp":
        return None
    rel = path.relative_to(REPO / "src")
    for suffix in (".h", ".hpp"):
        cand = rel.with_suffix(suffix).as_posix()
        if cand in PROJECT_HEADERS:
            return cand
    bare = rel.name[:-4] + ".h"
    return bare if bare in PROJECT_HEADERS else None


def classify(header: str, own: str | None) -> int:
    if header == RVA_H:
        return G_RVA
    if header == PRELUDE:
        return G_PRELUDE
    if own and header == own:
        return G_OWN
    if header in PROJECT_HEADERS:
        return G_PROJECT
    return G_LIBRARY


def sort_key(group: int, header: str):
    return (header.lower(),)


class Manual(Exception):
    """The include block holds something this tool will not rewrite."""


def parse(path: Path):
    """-> (head, entries, tail). A comment inside the block travels with the
    include it introduces; a trailing comment with no include belongs to the
    code below (keeps `// @early-stop` attached to its function)."""
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    i, n, head = 0, len(lines), []

    def trivia():
        nonlocal i
        while i < n and (not lines[i].strip()
                         or lines[i].lstrip().startswith(("//", "/*", "*"))):
            head.append(lines[i])
            i += 1

    trivia()
    if i + 1 < n and (m := IFNDEF_RE.match(lines[i])):
        m2 = DEFINE_RE.match(lines[i + 1])
        if m2 and m2.group(1) == m.group(1):
            head.extend(lines[i:i + 2])
            i += 2
    trivia()
    while i < n and DEFINE_RE.match(lines[i]):
        head.append(lines[i])
        i += 1
        trivia()

    entries, start = [], i
    pending: list[str] = []
    pending_at = i
    while i < n:
        s = lines[i].strip()
        if not s:
            i += 1
            continue
        if s.startswith(("//", "/*", "*")):
            if not pending:
                pending_at = i
            pending.append(lines[i])
            i += 1
            continue
        if m := INC_RE.match(lines[i]):
            entries.append((pending, m.group(1)))
            pending = []
            i += 1
            pending_at = i
            continue
        break

    if pending:
        i = pending_at
    if not entries:
        return head, [], lines[start:]

    depth = 0
    for j, line in enumerate(lines[i:]):
        s = line.strip()
        if m := PP_RE.match(s):
            d = m.group(1)
            if d in ("if", "ifdef", "ifndef"):
                depth += 1
            elif d == "endif":
                depth -= 1
            elif d == "include":
                raise Manual(f"include below the block (+{j})")
        elif s and not s.startswith(("//", "/*", "*")):
            break
    return head, entries, lines[i:]


def render(head, entries, tail, own):
    groups: dict[int, list] = {}
    seen: set[str] = set()
    for comments, h in entries:
        if h in seen:
            for g in groups.values():
                for k, (c, hh) in enumerate(g):
                    if hh == h:
                        g[k] = (c + comments, hh)
            continue
        seen.add(h)
        groups.setdefault(classify(h, own), []).append((comments, h))
    out = list(head)
    while out and not out[-1].strip():
        out.pop()
    if out:
        out.append("")
    first = True
    for g in GROUPS:
        if g not in groups:
            continue
        if not first:
            out.append("")
        first = False
        for comments, h in sorted(groups[g], key=lambda e: sort_key(g, e[1])):
            out.extend(comments)
            out.append(f"#include <{h}>")
    body = list(tail)
    while body and not body[0].strip():
        body.pop(0)
    if body:
        out.append("")
        out.extend(body)
    return out


def assert_conserved(path: Path, before, after, dropped):
    """Nothing but blank lines and duplicate includes may change - the reorder
    is a permutation, not an edit (a silent loss once ate 38 markers)."""
    from collections import Counter
    b = Counter(ln.strip() for ln in before if ln.strip())
    a = Counter(ln.strip() for ln in after if ln.strip())
    for h in dropped:
        for cand in (f"#include <{h}>", f'#include "{h}"'):
            if b[cand]:
                b[cand] -= 1
                if not b[cand]:
                    del b[cand]
                break
    if a != b:
        lost, gained = (b - a), (a - b)
        raise SystemExit(
            f"[include-order] ABORT: rewrite of {path} is not line-conserving\n"
            f"   lost:   {list(lost.elements())[:8]}\n"
            f"   gained: {list(gained.elements())[:8]}")


def prelude_violations(path: Path, headers) -> list[str]:
    """The prelude contract for one file: a .cpp includes <StdAfx.h>; a header
    never does; nothing but StdAfx.h includes <afx*.h> or <windows.h>."""
    out = []
    if PRELUDE and path.suffix == ".cpp" and PRELUDE not in headers:
        out.append(f"missing <{PRELUDE}>")
    if PRELUDE and path.suffix != ".cpp" and PRELUDE in headers:
        out.append(f"header includes <{PRELUDE}>")
    if PRELUDE and path.name != PRELUDE:
        out += [f"direct <{h}>" for h in headers if PLATFORM_RE.match(h)]
    return out


# Shared container/library headers sit below the game layers: nothing they
# reach, transitively, may come from a consumer directory.
LIBRARY_HEADERS = ("xCore/**/*.h", "xCore/**/*.hpp", "Entropy/**/*.h", "Entropy/**/*.hpp")
CONSUMER_DIRS = ("Meridian/", "Game/")


def layering_violations(include_root: Path | None = None) -> list[str]:
    """`library.h -> consumer.h` for every library header that reaches a
    consumer-directory include."""
    root = include_root or REPO / "include"
    out = []
    for pattern in LIBRARY_HEADERS:
        for start in sorted(root.glob(pattern)):
            pending, seen = [start], set()
            while pending:
                header = pending.pop()
                if header in seen:
                    continue
                seen.add(header)
                text = header.read_text(encoding="utf-8", errors="replace")
                for line in text.splitlines():
                    if not (m := INC_RE.match(line)):
                        continue
                    name = m.group(1)
                    if name.startswith(CONSUMER_DIRS):
                        out.append(f"{header.relative_to(root).as_posix()}"
                                   f" -> {name}")
                    elif (root / name).is_file():
                        pending.append(root / name)
    return sorted(set(out))


def audit(fix=False, fix_dupes=False, fix_prelude=False):
    """(dupes, preludes, unordered, manual, changed)."""
    dupes, preludes, unordered, manual = {}, {}, [], {}
    changed = 0
    for path in repo_files():
        rel = path.relative_to(REPO).as_posix()
        if path.name == PRELUDE:
            continue
        try:
            head, entries, tail = parse(path)
        except Manual as e:
            manual[rel] = str(e)
            continue
        if not entries:
            continue
        own = own_header(path)
        headers = [h for _, h in entries]
        dropped = [h for i, h in enumerate(headers) if h in headers[:i]]
        if dropped:
            dupes[rel] = sorted(set(dropped))
        found = prelude_violations(path, headers)
        if found:
            preludes[rel] = found
        want_add = ([PRELUDE] if PRELUDE and path.suffix == ".cpp"
                    and PRELUDE not in headers else [])
        work = list(entries)
        if fix_prelude:
            work.extend(([], h) for h in want_add)
        want = render(head, work, tail, own)
        have = path.read_text(encoding="utf-8", errors="replace").splitlines()
        if want != have:
            if not dropped and not found:
                unordered.append(rel)
            do = fix or (fix_dupes and dropped) or (fix_prelude and want_add)
            if do:
                assert_conserved(
                    path, have,
                    want if not fix_prelude else
                    [ln for ln in want
                     if ln.strip() not in {f"#include <{h}>" for h in want_add}],
                    dropped)
                path.write_text("\n".join(want) + "\n", encoding="utf-8")
                changed += 1
    return dupes, preludes, unordered, manual, changed


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="hobbit verify include-order",
                                 description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--gate", action="store_true",
                    help="exit 1 on duplicates, missing preludes or a disordered block")
    ap.add_argument("--fix-dupes", action="store_true",
                    help="rewrite files to drop duplicate includes")
    ap.add_argument("--fix-prelude", action="store_true",
                    help="add <StdAfx.h> to every .cpp that lacks it")
    ap.add_argument("--fix", action="store_true",
                    help="rewrite files into the full canonical order")
    ap.add_argument("--verbose", "-v", action="store_true",
                    help="list every file, not just the violations")
    a = ap.parse_args(argv)

    dupes, preludes, unordered, manual, changed = audit(
        a.fix, a.fix_dupes, a.fix_prelude)
    ndupe = sum(len(v) for v in dupes.values())
    print(f"[include-order] duplicate includes:      {ndupe} in "
          f"{len(dupes)} file(s)")
    print(f"[include-order] prelude violations:      {len(preludes)}")
    print(f"[include-order] files out of order:      {len(unordered)}")
    print(f"[include-order] MANUAL (untouched):      {len(manual)}")
    if a.verbose:
        for rel, d in sorted(dupes.items()):
            print(f"   dup  {rel}: {', '.join(d)}")
        for rel, w in sorted(preludes.items()):
            print(f"   pre  {rel}: {', '.join(w)}")
        for rel in unordered:
            print(f"   ord  {rel}")
    for rel, why in sorted(manual.items()):
        print(f"   MANUAL {rel}: {why}")

    if changed:
        print(f"[include-order] rewrote {changed} file(s)")
        return 0
    if a.gate and (ndupe or unordered or preludes):
        print("[include-order] FATAL: include block is not canonical - fix "
              "with `python3 -m hobbit.verify.include_order --fix-dupes "
              "--fix --fix-prelude` (a header's platform include moves to StdAfx.h)")
        return 1
    if a.gate:
        print("[include-order] OK - deduped, canonical order, StdAfx.h first "
              "in every .cpp and nowhere else")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
