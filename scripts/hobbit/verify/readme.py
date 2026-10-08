"""hobbit.verify.readme - the README score block + the one-time reconciliation.

The block between the markers is generated; nothing outside them is touched.
The first overwrite of an OLD-pipeline block (recognized by its generator
signature) emits a RECONCILIATION - the old block's numbers against the newly
computed ones, delta attributed per class - printed loudly, never written into
the README. The new numbers are the truth; they are not bent to match the old
block.
"""

from __future__ import annotations

import re

from hobbit.core.paths import REPO

README = REPO / "README.md"
RM_START = "<!-- match-score:start -->"
RM_END = "<!-- match-score:end -->"
OLD_SIGNATURE = "hobbit.match.status"     # the frozen pipeline's generator tag
NEW_SIGNATURE = "python3 -m hobbit.verify bank"


def _pct(num: float, den: float) -> float:
    return 100.0 * num / den if den else 0.0


def module_of(source: str) -> str:
    """Group units for the rollup by the meaningful path component."""
    from pathlib import PurePosixPath
    parts = PurePosixPath(source).parts
    if not parts:
        return "?"
    if parts[0] in ("src", "vendor") and len(parts) > 1:
        return parts[1]
    return parts[0]


def unit_modules() -> dict[str, str]:
    from hobbit import manifest
    return {u["unit"]: module_of(u.get("source", ""))
            for u in manifest.units()}


def _md_table(headers: list[str], aligns: str, rows: list[list[str]]) -> list[str]:
    widths = [len(h) for h in headers]
    for r in rows:
        for i, c in enumerate(r):
            widths[i] = max(widths[i], len(c))

    def cell(text: str, i: int) -> str:
        return text.rjust(widths[i]) if aligns[i] == "r" else text.ljust(widths[i])

    def row(cells: list[str]) -> str:
        return "| " + " | ".join(cell(c, i) for i, c in enumerate(cells)) + " |"

    sep = ["-" * (w - 1) + ":" if a == "r" else ":" + "-" * (w - 1)
           for w, a in zip(widths, aligns)]
    return [row(headers), "| " + " | ".join(sep) + " |", *(row(r) for r in rows)]


def collect_modules(umeas: dict[str, dict]):
    """Per-module aggregates from the (EH-carved) unit measures, plus the
    started-unit fuzzy-weighted sum + code."""
    modules = unit_modules()
    mods: dict[str, dict] = {}
    started_fzw = 0.0
    started_code = 0
    for unit, m in umeas.items():
        mod = modules.get(unit, "?")
        tc = int(m.get("total_code") or 0)
        fz = float(m.get("fuzzy_match_percent") or 0.0)
        a = mods.setdefault(mod, {"tc": 0, "mc": 0, "fzw": 0.0, "tf": 0,
                                  "mf": 0, "units": 0, "cw": 0.0})
        a["tc"] += tc
        a["mc"] += int(m.get("matched_code") or 0)
        a["fzw"] += fz * tc
        a["tf"] += int(m.get("total_functions") or 0)
        a["mf"] += int(m.get("matched_functions") or 0)
        a["units"] += 1
        started_fzw += fz * tc
        started_code += tc
    return mods, started_fzw, started_code


def target_rollup(doc, model, rvas):
    """Use one admitted reconstruction-target identity for all score sums.

    Compiler helpers and unresolved normalized symbols still get compared and
    banked, but must not inflate the numerator of the ordinary-function census.
    Their scored span count/bytes are displayed separately.
    """
    from copy import deepcopy
    from hobbit.verify import scores
    from hobbit.verify.universe import category
    target = {b.rva: b for b in model.functions if category(b) == "target"}
    filtered = deepcopy(doc)
    cur, sizes, seen = {}, {}, set()
    excluded_count = excluded_bytes = excluded_exact = 0
    for unit in filtered.get("units", []):
        unit_name = unit.get("name", "").split("/")[-1]
        kept, removed = [], []
        for row in unit.get("functions", []):
            key = (unit_name, row["name"])
            rva = rvas.get(key)
            binding = target.get(rva)
            if binding is None or rva in seen:
                removed.append(row)
                excluded_count += 1
                excluded_bytes += int(row.get("size") or 0)
                excluded_exact += scores.fn_fuzzy(row) >= scores.EXACT
                continue
            seen.add(rva)
            kept.append(row)
            cur[key] = scores.fn_fuzzy(row)
            sizes[key] = binding.size
        unit["functions"] = kept
        if removed:
            scores._subtract(unit.setdefault("measures", {}), removed)
    return filtered, cur, sizes, (excluded_count, excluded_bytes, excluded_exact)


def score_weights(cur: dict, ledger: dict, sizes: dict,
                  mods: dict, modules: dict) -> dict:
    """Fill each module's MAX exact count (`mx`) and MAX churn weight (`cw`,
    sum (MAX - CUR) * bytes), and return the whole-tree CUR/MAX/HIST exact
    counts and churn weights. `ledger` is the would-be banked ledger, so an
    edited function's MAX is its new CUR."""
    for a in mods.values():
        a["cw"], a["mx"] = 0.0, 0
    tot = {"cur": 0, "max": 0, "hist": 0, "cw": 0.0, "hw": 0.0}
    for key, pct in cur.items():
        row = ledger.get(key) or {}
        mx = max(row.get("best", pct), pct)
        hs = max(row.get("hist", mx), mx)
        size = sizes.get(key, 0)
        tot["cur"] += pct >= 100.0
        tot["max"] += mx >= 100.0
        tot["hist"] += hs >= 100.0
        tot["cw"] += (mx - pct) * size
        tot["hw"] += (hs - pct) * size
        mod = modules.get(key[0], "?")
        if mod in mods:
            mods[mod]["mx"] += mx >= 100.0
            mods[mod]["cw"] += (mx - pct) * size
    return tot


def render_block(mods: dict, started_fzw: float, eng: dict, tot: dict) -> str:
    """Render weighted MAX and the CUR/MAX/HIST ledger within the markers."""
    tot_fn, tot_code = eng["real_fn"], eng["real_code"]

    rows = []
    for mod in sorted(mods, key=lambda k: -mods[k]["tf"]):
        a = mods[mod]
        fz = (a["fzw"] + a["cw"]) / a["tc"] if a["tc"] else 0.0
        rows.append([f"`{mod}`", f"{a['units']}",
                     f"{a['mx']:,} / {a['tf']:,} ({_pct(a['mx'], a['tf']):.1f}%)",
                     f"{fz:.1f}%"])
    if eng["unmatched_fn"]:
        rows.append(["`(unmatched)`", "—",
                     f"0 / {eng['unmatched_fn']:,} (0.0%)", "0.0%"])
    table = _md_table(["Module", "Units", "Functions exact MAX", "Weighted MAX"],
                      "lrrr", rows)

    def weighted(extra: float) -> float:
        return (started_fzw + extra) / tot_code if tot_code else 0.0

    score_table = _md_table(
        ["Score", "Functions exact", "Weighted", "Meaning"], "lrrl",
        [["CUR", f"{tot['cur']:,}", f"{weighted(0.0):.2f}%", "last measured score"],
         ["MAX", f"{tot['max']:,}", f"{weighted(tot['cw']):.2f}%",
          "best result for each function's current source"],
         ["HIST", f"{tot['hist']:,}", f"{weighted(tot['hw']):.2f}%",
          "all-time peak across source revisions"]])
    other_count, other_bytes, other_exact = eng.get("other_scored", (0, 0, 0))
    block = [
        RM_START,
        "",
        f"**Windows `Meridian.exe`: {weighted(tot['cw']):.2f}% matched (MAX)** — "
        f"{tot['max']:,} / {tot_fn:,} functions exact "
        f"({_pct(tot['max'], tot_fn):.1f}%), weighted by size over "
        f"{tot_code:,} bytes of admitted reconstruction code.",
        "",
        *score_table,
        "",
        "MAX by module:",
        "",
        *table,
        "",
        f"Unclassified `.text`: {eng.get('unknown_code', 0):,} bytes outside the scores. "
        f"Other compared code: {other_count:,} spans / {other_bytes:,} bytes, "
        f"{other_exact:,} exact; excluded from the function denominator.",
        "",
        f"<!-- Generated by {NEW_SIGNATURE}; do not hand-edit. -->",
        RM_END,
    ]
    return "\n".join(block)


def current_block() -> str | None:
    text = README.read_text()
    if RM_START in text and RM_END in text:
        return text[text.index(RM_START): text.index(RM_END) + len(RM_END)]
    return None


def write_block(block: str) -> bool:
    """Replace the marked block; True when the README actually changed."""
    text = README.read_text()
    if RM_START in text and RM_END in text:
        pre = text[: text.index(RM_START)]
        post = text[text.index(RM_END) + len(RM_END):]
        new = pre + block + post
    else:
        i = text.find("\n## ")
        if i < 0:
            new = text.rstrip() + "\n\n" + block + "\n"
        else:
            new = text[:i] + "\n" + block + "\n" + text[i:]
    if new == text:
        return False
    README.write_text(new)
    return True


def old_block_numbers(block: str) -> dict | None:
    """Pull the headline numbers out of an OLD-pipeline block for the
    reconciliation. None when the block is not the old pipeline's."""
    if OLD_SIGNATURE not in block:
        return None
    out: dict = {}
    m = re.search(r"\*\*Overall \(vs ([^)]+)\): ([\d,]+) / ([\d,]+) functions "
                  r"exact \(([\d.]+)%\) &middot; ([\d.]+)% fuzzy "
                  r"&middot; ([\d.]+)% fuzzy max", block)
    if m:
        out.update(scope=m.group(1),
                   exact=int(m.group(2).replace(",", "")),
                   engine_fn=int(m.group(3).replace(",", "")),
                   exact_pct=float(m.group(4)), fuzzy=float(m.group(5)),
                   fuzzy_max=float(m.group(6)))
    m = re.search(r"Started units alone: ([\d,]+)/([\d,]+) fns exact, "
                  r"([\d.]+)% fuzzy over ([\d,]+) of ([\d,]+)", block)
    if m:
        out.update(started_fn=int(m.group(2).replace(",", "")),
                   started_fuzzy=float(m.group(3)),
                   started_code=int(m.group(4).replace(",", "")),
                   engine_code=int(m.group(5).replace(",", "")))
    m = re.search(r"`matched_data` \(([\d.]+)%\)", block)
    if m:
        out["matched_data_pct"] = float(m.group(1))
    for cat in ("EH unwind funclets", "private lifecycle/cleanup helpers",
                "static runtime/library", "jump thunks"):
        m = re.search(re.escape(f"`{cat}`") + r"\s*\|\s*([\d,]+)\s*\|\s*([\d,]+)",
                      block)
        if m:
            out[f"cat:{cat}"] = (int(m.group(1).replace(",", "")),
                                 int(m.group(2).replace(",", "")))
    return out
