"""hobbit.verify.data_coverage - is every datum MODELLED, not merely byte-equal?

The other half of the data-access audit, asked from OUR side. We choose the
extent, so a TOO-SMALL claim always scores 100: model `float g_v` where retail
has `struct { float x, y; }` and objdiff compares four bytes, finds them equal
and calls the section exact. This module reports which retail bytes NO enrolled
datum covers, and what retail's own payload says about them.

NEITHER SIDE ALONE TELLS PADDING FROM AN UNMODELLED FIELD - an uncovered byte
retail READS is unmodelled data, an uncovered byte nothing ever touches is
padding - so every row here carries the access map's verdict for the same
range (`touched`/`sites`, from hobbit.verify.data_access's sweep).

THE ORACLES, ALL FROM RETAIL, NONE FROM OUR PAYLOAD
  claims   the MODEL's named data bindings (rva + extent + owner) - every
           datum this tree claims, initialized or not. The frozen tool used
           the delink manifest alone, which predates the census: a `.bss`
           global that no object carves is claimed but never enrolled, so the
           manifest-only reading reported thousands of modelled globals as
           uncovered.
  enrolled build/gen/delink_data_manifest.tsv - the subset objdiff compares.
  sections build/gen/delink_data_section_manifest.tsv - a PLACED candidate
           section rebuilds (and objdiff compares) bytes no datum names: a /GR
           vtable COMDAT carries the ??_R4 pointer 4 bytes in front of `??_7`.
           Counting those as uncovered manufactured ~180 false positives along
           the vtable frontier alone in the frozen tool.
  payload  retail's bytes at an uncovered range. cl's inter-symbol padding is
           ZERO, so a NON-ZERO uncovered byte is content nobody modelled.
  .reloc   a relocated word INSIDE an uncovered range is a POINTER nobody
           modelled - conclusive, and it cannot be alignment padding; and the
           stored addresses give the set of addresses the image POINTS AT.

ADJACENCY PROVES NOTHING. A gap is reported, never closed by inventing an
aggregate to fill it.

VERDICTS
  PAD          uncovered, all-zero, shorter than 8 B. cl's own inter-symbol
               padding. Benign; the calibration floor.
  ZERO-GAP     uncovered, all-zero, too long to be padding.
  NONZERO      uncovered bytes that are not zero. Content nobody modelled.
  POINTER      NONZERO and retail relocates a word inside it - unmodelled
               pointer data, conclusive.
  OVERLAP      two claims covering one byte with different extents (a folded
               COMDAT seen from N objects agrees on both and is excluded).

    python3 -m hobbit.verify.data_coverage [--verdict V] [--near RVA]
    python3 -m hobbit.verify.data_coverage --tsv | --overlaps | --gate
"""

from __future__ import annotations

import bisect
from collections import Counter, defaultdict

from hobbit.core.paths import BUILD
from hobbit.core.tsv import read as read_tsv
from hobbit.core.tsv import write as write_tsv

MANIFEST = BUILD / "gen/delink_data_manifest.tsv"
SECTIONS = BUILD / "gen/delink_data_section_manifest.tsv"
GAPS_TSV = BUILD / "gen/data_coverage_gaps.tsv"

#: cl aligns a standalone global to its own element size, capped at 8 for the
#: ordinary sections (16 needs `__declspec(align)`, which MSVC 5 lacks).
MAX_PAD = 8

GAP_COLUMNS = ["rva", "length", "section", "verdict", "addressed", "touched",
               "sites", "payload_nonzero", "relocs", "prev_object",
               "prev_name", "next_object", "next_name", "first_bytes", "ownership_scope", "owner_units"]


def _optional_int(value):
    """Missing/malformed contribution metadata cannot establish scope."""
    try:
        return int(value, 0)
    except (TypeError, ValueError):
        return None


def _independent_section(section):
    """A complete placed non-associative SELECT_ANY contribution.

    Its exact extent contributes coverage; it cannot establish ownership of
    the bytes outside that extent merely because another contribution from
    the same consuming object occurs nearby. This is a Hobbit adaptation of
    the donor coverage gate, based on validated candidate COFF metadata.
    """
    return (section.get("provenance") == "candidate-COFF-section"
            and isinstance(section.get("ordinal"), int)
            and section["ordinal"] > 0
            and section.get("size", 0) > 0
            and section.get("comdat_selection") == 2
            and bool((section.get("characteristics") or 0) & 0x1000)
            and section.get("associative_ordinal") == "-")


def _boundary_claims(claims, sections):
    """Keep all witnesses and join model/manifest duplicates by exact identity.

    An unrelated ordinary symbol at the same address is not a duplicate.
    Conflicting or missing manifest metadata retains the ordinary witness.
    """
    bysection, byidentity = defaultdict(list), defaultdict(list)
    for section in sections:
        bysection[(section["object"], section.get("ordinal"))].append(section)

    def identity(claim):
        return tuple(claim.get(k) for k in
                     ("object", "name", "rva", "size", "storage"))

    def complete_contribution(claim):
        candidates = bysection.get((claim["object"],
                                    claim.get("section_ordinal")), [])
        provenance = claim.get("provenance") or ""
        if (not candidates or claim.get("section_offset") != 0
                or not (provenance.startswith("candidate-COFF-")
                        or provenance == "src-DATA-sizeof")):
            return False
        return all(_independent_section(section)
                   and all(claim.get(k) == section.get(k)
                           for k in ("object", "rva", "size", "storage"))
                   for section in candidates)

    for claim in claims:
        if claim.get("origin") == "manifest":
            byidentity[identity(claim)].append(claim)
    result = []
    for claim in claims:
        peers = byidentity.get(identity(claim), [])
        independent = (claim.get("origin") in ("model", "manifest")
                       and bool(peers)
                       and all(complete_contribution(p) for p in peers))
        result.append(dict(claim, independent_contribution=independent))
    for section in sections:
        consistent = all(section == peer for peer in
                         bysection[(section["object"], section.get("ordinal"))])
        result.append(dict(section, name=f"<section {section['name']}>",
                           independent_contribution=(consistent
                                                     and _independent_section(section))))
    return result


def _ownership(prev, nxt):
    ordinary_prev = {c["object"] for c in prev
                     if not c["independent_contribution"]}
    ordinary_next = {c["object"] for c in nxt
                     if not c["independent_contribution"]}
    owners = sorted((ordinary_prev & ordinary_next) - FRONTIER_UNITS)
    if owners:
        return "same-unit-boundaries", owners
    if any(c["independent_contribution"] for c in prev + nxt):
        return "unresolved-inter-contribution", []
    return "unresolved-frontier", []


def load_claims(path=MANIFEST):
    """Every enrolled datum. The same retail extent appears once per object
    that defines it (a folded COMDAT literal or vtable), so a caller needing
    the covered byte SET must dedupe on (rva, size) - `coverage()` does."""
    if not path.is_file():
        raise SystemExit(f"no {path} - run `hobbit build` first")
    _b, _h, rows = read_tsv(path)
    return [{"name": r["name"], "object": r["object"], "storage": r["storage"],
             "rva": int(r["rva"], 16), "size": int(r["size"], 16),
             "section_ordinal": _optional_int(r.get("section_ordinal")),
             "section_offset": _optional_int(r.get("section_offset")),
             "provenance": r.get("provenance"), "origin": "manifest"}
            for r in rows]


def load_sections(path=SECTIONS):
    """Every PLACED candidate section - the other half of what objdiff
    compares. A `-` rva is a non-affine section with no retail claim."""
    if not path.is_file():
        return []
    _b, _h, rows = read_tsv(path)
    return [{"object": r["object"], "name": r["name"],
             "rva": int(r["rva"], 16), "size": int(r["size"], 16),
             "storage": r["storage"],
             "ordinal": _optional_int(r.get("ordinal")),
             "characteristics": _optional_int(r.get("characteristics")),
             "comdat_selection": _optional_int(r.get("comdat_selection")),
             "associative_ordinal": r.get("associative_ordinal"),
             "provenance": r.get("provenance")}
            for r in rows if r["rva"] != "-"]


def coverage(claims, sections=()):
    """Maximal covered runs [start, end): a byte an enrolled datum claims OR a
    placed candidate section rebuilds. Only what neither reaches is unmodelled."""
    ext = {(c["rva"], c["size"]) for c in claims}
    ext |= {(s["rva"], s["size"]) for s in sections}
    merged: list[list[int]] = []
    for a, sz in sorted(ext):
        b = a + sz
        if merged and a <= merged[-1][1]:
            merged[-1][1] = max(merged[-1][1], b)
        else:
            merged.append([a, b])
    return [(a, b) for a, b in merged]


def overlaps(claims):
    """Claims covering a shared byte with DIFFERENT extents. One retail COMDAT
    defined by N objects agrees on rva AND size, so it is not an overlap; a
    genuine overlap means at least one of the two extents is the wrong shape."""
    extents = sorted({(c["rva"], c["size"]) for c in claims})
    byext = defaultdict(list)
    for c in claims:
        byext[(c["rva"], c["size"])].append(c)
    out = []
    for i, (a, sz) in enumerate(extents):
        for a2, sz2 in extents[i + 1:]:
            if a2 >= a + sz:
                break
            out.append(((a, sz, byext[(a, sz)]), (a2, sz2, byext[(a2, sz2)])))
    return out


def touched_index():
    """(starts, ends, sites) of the byte ranges retail's code TOUCHES, from the
    access map's own sweep - the join that turns a gap into a verdict."""
    from hobbit.verify import data_access as da
    spine, accesses, *_rest = da.analysis()
    rows = [(a.target_rva, a.end_rva) for a in accesses
            if a.form in da.TOUCH and a.width]
    return (spine, *_touch_ranges(rows))


def _touch_ranges(rows):
    """Union bytes, but retain exact access endpoints for intersection counts."""
    rows = sorted(rows)
    starts, ends = [], []
    for lo, hi in rows:
        if starts and lo <= ends[-1]:
            ends[-1] = max(ends[-1], hi)
            continue
        starts.append(lo)
        ends.append(hi)
    sites = (sorted(lo for lo, _hi in rows),
             sorted(hi for _lo, hi in rows))
    return starts, ends, sites


def _touched(starts, ends, sites, lo, hi):
    """(bytes touched inside [lo, hi), count of intersecting accesses)."""
    i = max(bisect.bisect_right(starts, lo) - 1, 0)
    n = (bisect.bisect_left(sites[0], hi)
         - bisect.bisect_right(sites[1], lo)) if sites else 0
    tb = 0
    while i < len(starts) and starts[i] < hi:
        a, b = max(starts[i], lo), min(ends[i], hi)
        if b > a:
            tb += b - a
        i += 1
    return tb, n


def _name_key(claim):
    return (claim["name"].startswith("<"), claim["name"])


def gaps(img, claims, sections=(), touched=None):
    """Every uncovered range strictly between two covered runs, with a verdict.

    Ranges before the first claim and after the last are the library frontier,
    not a modelling defect: territory nobody has attributed, excluded so the
    signal is not drowned."""
    runs = coverage(claims, sections)
    ends, starts = defaultdict(list), defaultdict(list)
    for c in _boundary_claims(claims, sections):
        starts[c["rva"]].append(c)
        ends[c["rva"] + c["size"]].append(c)

    tstarts, tends, tsites = touched or ([], [], [])
    out = []
    for (_a1, b1), (a2, _b2) in zip(runs, runs[1:]):
        n = a2 - b1
        pay = img.read(b1, n) or b""
        nz = sum(1 for x in pay if x)
        rel = img.relocs_in(b1, b1 + n)
        ptd = img.refs_to_range(b1, b1 + n)
        # a real datum names the boundary better than the section holding it
        prev = sorted(ends.get(b1, []), key=_name_key)
        nxt = sorted(starts.get(a2, []), key=_name_key)
        scope, owners = _ownership(prev, nxt)
        tb, tn = _touched(tstarts, tends, tsites, b1, a2)
        if rel and nz:
            verdict = "POINTER"
        elif nz:
            verdict = "NONZERO"
        elif n < MAX_PAD:
            verdict = "PAD"
        else:
            verdict = "ZERO-GAP"
        out.append({
            "rva": b1, "length": n, "section": img.section_name(b1),
            "verdict": verdict, "addressed": len({t for _s, t in ptd}),
            "touched": tb, "sites": tn, "payload_nonzero": nz,
            "relocs": len(rel),
            "prev_object": prev[0]["object"] if prev else "-",
            "prev_name": prev[0]["name"] if prev else "-",
            "next_object": nxt[0]["object"] if nxt else "-",
            "next_name": nxt[0]["name"] if nxt else "-",
            "first_bytes": pay[:16].hex(),
            "ownership_scope": scope, "owner_units": owners})
    return out


def model_claims(spine):
    """The Model's named data bindings in `load_claims()` shape - the claim
    authority (the manifest is only what objdiff got to compare)."""
    return [{"name": c.name, "object": c.unit, "storage": c.space,
             "rva": c.rva, "size": c.extent, "origin": "model"}
            for c in spine.claims]


def census():
    """(rows, claims, sections, img) - the whole census, one sweep."""
    spine, ts, te, tn = touched_index()
    claims = model_claims(spine) + load_claims()
    sections = load_sections()
    return gaps(spine.img, claims, sections, (ts, te, tn)), claims, sections, \
        spine.img


#: units whose "territory" is the library frontier, not ours
FRONTIER_UNITS = frozenset({"library_data", "-", ""})


def gate_rows(rows):
    """The FATAL subset. Four conditions, each one earning its place:

      touched     retail's own code reads or writes those bytes, so they are
                  not alignment padding;
      NONZERO/    retail's payload there is content, and cl's inter-symbol
      POINTER     padding is zero;
      same unit   ordinary boundary witnesses share a live unit. Complete,
                  independently placed COMDAT contributions establish only
                  their own extents, never ownership between contributions.
                  Unsupported ownership remains in the unresolved worklist;
      not .idata  the import tables are the linker's storage; a source pin on
                  an IAT slot is the access map's `import-slot` finding, and
                  double-reporting it here would just make two gates red for
                  one defect.
    """
    return [r for r in rows
            if r["touched"] and r["verdict"] in ("NONZERO", "POINTER")
            and r["section"] != ".idata"
            and r.get("owner_units", [r["prev_object"]]
                      if r["prev_object"] == r["next_object"]
                      and r["prev_object"] not in FRONTIER_UNITS else [])]


def gate_findings() -> list[str]:
    rows, claims, _sections, _img = census()
    out = [f"data-coverage: 0x{r['rva']:06x}+0x{r['length']:x} {r['verdict']} "
           f"is touched by retail ({r['sites']} site(s), {r['touched']} B) but "
           f"no claim of {', '.join(r['owner_units'])} covers it - between "
           f"{r['prev_name'][:40]} and {r['next_name'][:40]}"
           for r in gate_rows(rows)]
    for (a, sz, c1), (a2, sz2, c2) in overlaps(claims):
        out.append(f"data-coverage: OVERLAP 0x{a:06x}+0x{sz:x} "
                   f"{c1[0]['name'][:40]} vs 0x{a2:06x}+0x{sz2:x} "
                   f"{c2[0]['name'][:40]} - one extent is the wrong shape")
    from hobbit.verify.tiers import ScopedFindings
    return ScopedFindings(out, "claimed data territory only; whole-image reference inventory remains partial")


def _summary(rows, claims, sections):
    tally = Counter(r["verdict"] for r in rows)
    by = Counter()
    touched = Counter()
    for r in rows:
        by[r["verdict"]] += r["length"]
        touched[r["verdict"]] += r["touched"]
    covered = sum(b - a for a, b in coverage(claims, sections))
    print("reference coverage: partial reviewed /FIXED fields; zero reference counts do not prove absence")
    print(f"enrolled claims {len(claims)} "
          f"({len({(c['rva'], c['size']) for c in claims})} distinct extents) "
          f"+ {len(sections)} placed sections, covering {covered} B")
    print(f"uncovered interior ranges: {len(rows)}, {sum(by.values())} B")
    for v in ("POINTER", "NONZERO", "ZERO-GAP", "PAD"):
        print(f"  {v:<9} {tally[v]:5} ranges  {by[v]:9} B  "
              f"{sum(r['addressed'] for r in rows if r['verdict'] == v):5} "
              f"addressed  {touched[v]:7} B TOUCHED by retail")


def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="hobbit verify data-coverage",
                                 description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--tsv", nargs="?", const=GAPS_TSV,
                    help="write the join-shaped gap census")
    ap.add_argument("--overlaps", action="store_true",
                    help="list claims whose extents share a byte, and exit")
    ap.add_argument("--verdict",
                    help="only rows with this verdict (PAD/ZERO-GAP/NONZERO/POINTER)")
    ap.add_argument("--touched-only", action="store_true",
                    help="only gaps retail's code actually reads or writes")
    ap.add_argument("--min-len", type=int, default=0,
                    help="ignore gaps shorter than this many bytes")
    ap.add_argument("--near", help="claims and payload around one rva")
    ap.add_argument("--gate", action="store_true",
                    help="exit 1 on a gated gap or an overlap")
    ap.add_argument("--limit", type=int, default=40,
                    help="cap the printed worklist")
    a = ap.parse_args(argv)

    if a.near:
        return _near(int(a.near, 0))

    rows, claims, sections, _img = census()
    if a.overlaps:
        ov = overlaps(claims)
        print(f"overlapping claims (different extents sharing a byte): "
              f"{len(ov)}")
        for (a1, s1, c1), (a2, s2, c2) in ov[:a.limit]:
            print(f"  0x{a1:06x}+0x{s1:<5x} {c1[0]['name'][:44]:44} "
                  f"[{c1[0]['object']}]")
            print(f"  0x{a2:06x}+0x{s2:<5x} {c2[0]['name'][:44]:44} "
                  f"[{c2[0]['object']}]")
        return 1 if ov else 0

    if a.tsv:
        changed = write_tsv(
            a.tsv, ["# GENERATED by hobbit.verify.data_coverage - retail bytes "
                    "no enrolled datum covers, joined with the access map."],
            GAP_COLUMNS,
            [[f"0x{r['rva']:06x}", r["length"], r["section"], r["verdict"],
              r["addressed"], r["touched"], r["sites"], r["payload_nonzero"],
              r["relocs"], r["prev_object"], r["prev_name"], r["next_object"],
              r["next_name"], r["first_bytes"], r["ownership_scope"],
              "|".join(r["owner_units"])] for r in rows])
        print(f"wrote {a.tsv} ({len(rows)} rows, join key rva+length, "
              f"{'updated' if changed else 'unchanged'})")

    _summary(rows, claims, sections)
    sel = [r for r in rows if r["length"] >= a.min_len
           and (not a.verdict or r["verdict"] == a.verdict)
           and (not a.touched_only or r["touched"])]
    print("\nworklist (verdict != PAD, TOUCHED first, then addressed):")
    work = sorted([r for r in sel if r["verdict"] != "PAD"],
                  key=lambda r: (-r["touched"], -r["addressed"],
                                 -r["payload_nonzero"]))
    for r in work[:a.limit]:
        print(f"  0x{r['rva']:06x} len={r['length']:<6} {r['verdict']:<8} "
              f"touched={r['touched']:<5} addr={r['addressed']:<4} "
              f"nz={r['payload_nonzero']:<5} rel={r['relocs']:<4} "
              f"{r['prev_object']} | {r['next_object']} "
              f"[{r['ownership_scope']}]")
        print(f"      {r['prev_name'][:46]:46} -> {r['next_name'][:46]}")
    if a.gate:
        bad = gate_findings()
        for b in bad:
            print(b)
        return 1 if bad else 0
    return 0


def _near(rva, span=0x80):
    """The claims, sections and retail payload around one address."""
    from hobbit.sema.image import retail
    img = retail()
    claims, sections = load_claims(), load_sections()
    lo, hi = rva - span, rva + span
    print(f"=== claims overlapping [0x{lo:06x}, 0x{hi:06x}) ===")
    for c in sorted(claims, key=lambda c: (c["rva"], c["name"])):
        if c["rva"] + c["size"] <= lo or c["rva"] >= hi:
            continue
        print(f"  0x{c['rva']:06x}+0x{c['size']:<5x} {c['storage']:<8} "
              f"{c['name'][:46]:46} [{c['object']}]")
    hits = [s for s in sections if s["rva"] < hi and s["rva"] + s["size"] > lo]
    if hits:
        print("=== placed candidate sections here ===")
        for s in sorted(hits, key=lambda s: s["rva"]):
            print(f"  0x{s['rva']:06x}+0x{s['size']:<5x} {s['name']:<10} "
                  f"{s['object']}")
    print("=== retail payload ===")
    pay = img.read(lo, hi - lo) or b""
    rel = {s for s, _t in img.relocs_in(lo, hi)}
    ptd = {t for _s, t in img.refs_to_range(lo, hi)}
    for off in range(0, len(pay), 16):
        at = lo + off
        marks = "".join("R" if (at + i) in rel else
                        "*" if (at + i) in ptd else " " for i in range(16))
        print(f"  0x{at:06x}  {pay[off:off + 16].hex(' '):47}  |{marks}|")
    print("  (R = retail relocates this word, * = something points AT it)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
