"""hobbit.verify.verbs - status / check / bank.

status  reports and exits 0. check is the MAX gate: nonzero on a REAL
regression - a FRESH below-bank dip (new since the committed snapshot), an
unbanked LOST body, or a hard report failure (a dummy.obj pairing's zero-total
100%). Carried (inherited) debt is reported, preserved, and owned by the walls
worklist, not re-failed on every run. bank is a MANUAL act: nothing regenerates
the baseline automatically.
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

from hobbit.core.paths import REPO
from hobbit.verify import baseline as bl
from hobbit.verify import classify as cl
from hobbit.verify import readme as rm
from hobbit.verify import scores
from hobbit.verify.baseline import EPS
from hobbit.verify.fingerprints import fingerprinter, is_fallback, real_edit

# Every repository path that can change compilation, delinking, or the
# measurement itself. A score snapshot may be committed beside STAGED source
# (the index is then an explicit provenance snapshot), but never silently
# written from an unstaged working-tree experiment. README.md and the baseline
# are deliberately absent: they are the outputs this check protects.
BANK_INPUT_PATHS = (
    "src", "include", "vendor", "config", "scripts/hobbit", "configure.py",
    "flake.nix", "flake.lock", "nix", "recomp", "tools",
)
BANK_OUTPUTS = {"config/match_baseline.tsv"}


def unstaged_bank_inputs() -> list[str]:
    commands = (
        ["git", "diff", "--name-only", "--", *BANK_INPUT_PATHS],
        ["git", "ls-files", "--others", "--exclude-standard", "--",
         *BANK_INPUT_PATHS],
    )
    paths: set[str] = set()
    for command in commands:
        result = subprocess.run(command, cwd=str(REPO), capture_output=True,
                                text=True)
        if result.returncode != 0:
            raise SystemExit(
                "cannot verify score-banking provenance: "
                + (result.stderr.strip() or "git status query failed"))
        paths.update(line for line in result.stdout.splitlines() if line)
    return sorted(paths - BANK_OUTPUTS)


def require_bankable_tree(action: str, allow_dirty: bool = False) -> None:
    dirty = unstaged_bank_inputs()
    if not dirty:
        return
    shown = "\n".join(f"  {path}" for path in dirty[:12])
    more = f"\n  ... and {len(dirty) - 12} more" if len(dirty) > 12 else ""
    if allow_dirty:
        print(f"WARNING: banking over unstaged/untracked build inputs "
              f"(--dirty given):\n{shown}{more}", file=sys.stderr)
        return
    raise SystemExit(
        f"refusing to {action} from unstaged/untracked build inputs:\n"
        f"{shown}{more}\n"
        "stage the intended build-input changes first (or commit them), "
        "rebuild, then bank beside that exact source snapshot "
        "(--dirty overrides, loudly)")


# --------------------------------------------------------------------------- #
# shared state                                                                #
# --------------------------------------------------------------------------- #
def load_state(report=None):
    doc = scores.load(report)
    cur = scores.functions(doc)
    base_funcs = bl.load()
    fp, _cpp_of, stale = fingerprinter()
    rvas = cl.model_rvas()
    return doc, cur, base_funcs, fp, stale, rvas


def _warn_stale(stale: set) -> None:
    if stale:
        names = ", ".join(sorted(stale)[:8]) + (" ..." if len(stale) > 8 else "")
        print(f"WARNING: fingerprint cache stale/absent for {len(stale)} "
              f"unit(s) ({names}) - edit detection degraded; refresh with "
              f"`python3 -m hobbit.verify fingerprints`.", file=sys.stderr)


def _report_object_age(report=None):
    """Observed report age relative to native objects; shared warning contract."""
    try:
        path = Path(report) if report else scores.report_path()
        newest = max((p.stat().st_mtime
                      for p in (REPO / "build/objdiff/base").glob("*.obj")),
                     default=0.0)
        return path, newest - path.stat().st_mtime
    except (OSError, SystemExit):
        return None


def _warn_stale_report(report=None) -> None:
    """Warn when a report predates an observed native object by over a second."""
    observed = _report_object_age(report)
    if observed is not None and observed[1] > 1.0:
        path, age = observed
        print(f"WARNING: {path.name} is STALE - a base obj is {age:.0f}s "
              f"newer, so these scores describe the PREVIOUS build. Re-run "
              f"`hobbit build` before reading (or banking) them.",
              file=sys.stderr)


def walls_style_counts(cur, base_funcs, rvas):
    """The strict-`<` below-bank count (rva-joined, no EPS), split into
    sub-EPS float jitter vs real, as a cross-check on the EPS-gated figure.

    NOT walls.inventory's own predicate: that one is `pct < bank - EPS`
    (hobbit.walls.inventory.build). This is deliberately the WIDER reading -
    it counts the quantization jitter the gate ignores, so the two numbers
    bracket the real movement.
    """
    by_rva = cl.index_by_rva(base_funcs)
    strict = jitter = 0
    for key, pct in cur.items():
        if pct >= 100.0:
            continue
        addr = rvas.get(key)
        old = by_rva.get(addr) if addr is not None else None
        if old is None:
            continue
        best = base_funcs[old]["best"]
        if pct < best:
            strict += 1
            if best - pct <= EPS:
                jitter += 1
    return strict, jitter


# --------------------------------------------------------------------------- #
# status / check                                                              #
# --------------------------------------------------------------------------- #
def _show(kind, rows, note=""):
    if not rows:
        return
    print(f"\n{kind} ({len(rows)}){note}:")

    def key(r):
        return (r[2] - r[3]) if (r[2] is not None and r[3] is not None) else 0

    for u, f, p, b in sorted(rows, key=key):
        if p is None:
            print(f"  {u:<18} {f}\n      MAX {b:.4f} (preserved)  now absent")
        elif b is None:
            print(f"  {u:<18} {f}   now {p:.4f}")
        else:
            print(f"  {u:<18} {f}\n      MAX {b:.4f} (held) -> now {p:.4f}   "
                  f"(delta {p - b:+.4f})")


def _report(args, gate: bool) -> int:
    doc, cur, base_funcs, fp, stale, rvas = load_state(args.report)
    _warn_stale_report(args.report)
    fails = scores.hard_failures(doc)
    if not base_funcs:
        fails.append("no baseline (config/match_baseline.tsv) - seed it: "
                     "python3 -m hobbit.verify bank")
    if stale:
        _warn_stale(stale)

    buckets = cl.buckets_of(cur, base_funcs, fp, rvas) if base_funcs else {}
    regress = buckets.get("REGRESS", [])
    lost = buckets.get("LOST", [])
    cy = cl.currency(cur, base_funcs, regress)
    fresh = cl.fresh_regressions(cur, base_funcs, regress)
    carried = [r for r in regress if r not in fresh]
    improve = buckets.get("IMPROVE", [])
    strict_below, jitter = walls_style_counts(cur, base_funcs, rvas)

    m = doc.get("measures", {})
    exact = int(m.get("matched_functions") or 0)
    total = int(m.get("total_functions") or 0)
    print(f"scored {total} function(s) (EH band carved) - {exact} exact, "
          f"overall fuzzy {float(m.get('fuzzy_match_percent') or 0.0):.2f}%")
    print(f"below the banked MAX (beyond EPS={EPS}): "
          f"{len(regress)} REGRESS ({len(carried)} carried, {len(fresh)} fresh), "
          f"{len(buckets.get('RESET', []))} RESET, "
          f"{len(buckets.get('DIP', []))} DIP (MAX held); "
          f"strict-< count {strict_below} ({jitter} sub-EPS float jitter)")

    for kind, note in (("REGRESS", " (edited, CUR fell below the banked MAX)"),
                       ("RESET", " (edited, CUR held; the new hash lowers MAX)"),
                       ("DIP", " (unedited, CUR dip; MAX held, no action)"),
                       ("LOST", " (rva no longer scored, not banked absent)"),
                       ("IMPROVE", " (bankable: cur > best)"),
                       ("MOVED", " (same rva, new unit - transfers)"),
                       ("RENAMED", " (same rva, new name)"),
                       ("NEW", ""), ("REMOVED", " (rename/edit adjudicated)"),
                       ("KNOWN-ABSENT", " (banked absent, MAX preserved)")):
        rows = buckets.get(kind, [])
        if kind in ("MOVED", "RENAMED", "REMOVED", "KNOWN-ABSENT", "DIP") \
                and not getattr(args, "all", False):
            if rows:
                print(f"\n{kind}: {len(rows)} row(s){note} (--all lists them)")
            continue
        _show(kind, rows, note)

    if cy["snapshot_drift"]:
        print(f"\nBASELINE CURRENCY: {cy['snapshot_drift']} of "
              f"{cy['compared_rows']} rows differ from the banked cur_pct "
              f"snapshot; of the {len(regress)} REGRESS, "
              f"{cy['regress_carried']} were already below best in it "
              f"(inherited) and {cy['regress_fresh']} are new since it.")
    for f in fails:
        print(f"\nHARD FAILURE: {f}")

    if gate:
        strict_extra = args.strict and (carried or lost)
        bad = bool(fails or fresh or lost or strict_extra)
        if bad:
            why = []
            if fails:
                why.append(f"{len(fails)} hard failure(s)")
            if fresh:
                why.append(f"{len(fresh)} fresh regression(s)")
            if lost:
                why.append(f"{len(lost)} unbanked loss(es)")
            if args.strict and carried:
                why.append(f"{len(carried)} carried regression(s) [--strict]")
            print(f"\nCHECK FAILED: {', '.join(why)}. The banked MAX is "
                  f"preserved for every row above; fix the source (never the "
                  f"ledger), or adjudicate + bank.")
            return 1
        note = f" ({len(improve)} unbanked improvement(s) - run bank)" \
            if improve else ""
        print(f"\ncheck OK: no fresh regressions vs the banked MAX.{note}")
        return 0
    return 0


def cmd_status(argv) -> int:
    ap = argparse.ArgumentParser(
        prog="hobbit verify status",
        description="report-only; exits 0. (`--strict` is a CHECK flag: it "
                    "changes an exit code, and status has none.)")
    ap.add_argument("--report", type=str, default=None,
                    help="an objdiff report.json (default: "
                         "build/objdiff/compare-new/report.json)")
    ap.add_argument("--all", action="store_true",
                    help="list MOVED/RENAMED/REMOVED/KNOWN-ABSENT rows too")
    a = ap.parse_args(argv)
    a.report = Path(a.report) if a.report else None
    a.strict = False          # status never gates; _report reads the field
    _report(a, gate=False)
    return 0



def refresh_readme_block(report=None) -> bool:
    """Re-render the derived block from the comparison report and ledger.

    Follow Gruntz verify/verbs.py at 7d4bd55b99e32f084834d991badf7609889481f4:
    rendering does not gate on native-object age or fingerprint freshness.
    The build graph refreshes measurement inputs before this edge; standalone
    rendering describes the supplied report. The banked ledger stays manual.
    """
    from hobbit.model import resolve
    from hobbit.verify.universe import engine_universe
    doc, cur, base, fp, _stale, rvas = load_state(report)
    model = resolve()
    target_doc, target_cur, sizes, other = rm.target_rollup(doc, model, rvas)
    umeas = scores.unit_measures(target_doc)
    mods, started_fzw, _started_code = rm.collect_modules(umeas)
    ledger, *_ = bank_rows(cur, base, fp, rvas)
    tot = rm.score_weights(target_cur, ledger, sizes, mods, rm.unit_modules())
    eng = engine_universe(model)
    eng["other_scored"] = other
    block = rm.render_block(mods, started_fzw, eng, tot)
    return rm.write_block(block)


def cmd_readme(argv) -> int:
    """Re-render README's score block from the current report and ledger."""
    argparse.ArgumentParser(prog="hobbit verify readme",
                            description=cmd_readme.__doc__).parse_args(argv)
    changed = refresh_readme_block()
    print(f"README score block {'refreshed' if changed else 'unchanged'}")
    return 0


def cmd_check(argv) -> int:
    ap = argparse.ArgumentParser(prog="hobbit verify check",
                                 description="the MAX gate + the tiered gates")
    ap.add_argument("--report", type=str, default=None,
                    help="an objdiff report.json (default: "
                         "build/objdiff/compare-new/report.json)")
    ap.add_argument("--all", action="store_true",
                    help="list MOVED/RENAMED/REMOVED/KNOWN-ABSENT rows too")
    ap.add_argument("--strict", action="store_true",
                    help="also fail on carried (inherited) regressions")
    ap.add_argument("--tier", type=str, default=None,
                    help="comma list of gate tiers to run after the MAX gate: "
                         "fast|normal|full|link, or 'none' (default: "
                         "fast,normal - what the graph's check edge runs; "
                         "full/link are opt-in)")
    ap.add_argument("--no-readme", action="store_true",
                    help="do not refresh README's derived score block")
    a = ap.parse_args(argv)
    a.report = Path(a.report) if a.report else None
    # validate --tier BEFORE the MAX gate: an unknown tier used to be reported
    # only after the whole run, behind a wall of regression output.
    from hobbit.verify import tiers
    tier_names = tiers.parse_tiers(a.tier)
    rc = _report(a, gate=True)
    if not a.no_readme and refresh_readme_block(a.report):
        print(f"README score block refreshed ({rm.README.relative_to(REPO)})")
    failed = tiers.run(tier_names)
    if failed:
        print(f"\nTIER GATES FAILED: {failed} gate(s) - fix the finding, "
              f"never weaken the gate (baselined debt is carried by each "
              f"gate's own committed floor).")
    return 1 if (rc or failed) else 0


# --------------------------------------------------------------------------- #
# bank                                                                        #
# --------------------------------------------------------------------------- #
def library_rvas() -> set[int]:
    from hobbit.model import resolve
    return {b.rva for b in resolve().functions
            if b.channel == "functions_static_libs"}


def bank_rows(cur, base_funcs, fp, rvas):
    """The ported update: raise best where cur > best under the src_hash
    rules, reset where the hash changed, rebound where the rva moved, migrate
    the high-water by rva across unit moves, preserve absent bodies."""
    base_by_addr = cl.index_by_rva(base_funcs)
    stats = {"raised": 0, "touched": 0, "added": 0, "rebounds": 0, "moved": 0,
             "preserved_absent": 0}
    reset_by_edit: list = []
    migrated_old_keys: set = set()
    new_funcs: dict = {}
    for key, pct in cur.items():
        cur_fp = fp(*key)
        prev = base_funcs.get(key)
        cur_addr = rvas.get(key)
        if prev is None and cur_addr is not None and cur_addr in base_by_addr:
            old_key = base_by_addr[cur_addr]
            prev = base_funcs[old_key]
            if old_key != key:
                migrated_old_keys.add(old_key)
                stats["moved"] += 1
        if prev is None:
            new_funcs[key] = {"best": pct, "cur": pct, "tries": 1,
                              "fp": cur_fp, "hist": pct, "state": ""}
            stats["added"] += 1
        elif cl.rebound(prev.get("addr"), cur_addr):
            # the NAME now labels a DIFFERENT BODY: the old peak measured the
            # body that used to sit here - start fresh (the only rva-keyed
            # path that lowers a best; hist carries the all-time mark).
            new_funcs[key] = {"best": pct, "cur": pct, "tries": 1,
                              "fp": cur_fp, "state": "",
                              "hist": max(prev.get("hist", prev["best"]), pct)}
            stats["rebounds"] += 1
        else:
            # SAME BODY: same src_hash + a different % banks the high mark
            # (TU composition moved, not the source); a CHANGED src_hash
            # resets best to cur (the old peak belonged to source that no
            # longer exists - the new source has not earned it).
            keep_fp = cur_fp if not is_fallback(cur_fp) else prev["fp"]
            edited = real_edit(prev["fp"], cur_fp)
            best = pct if edited else max(prev["best"], pct)
            new_funcs[key] = {
                "best": best, "cur": pct,
                "tries": prev["tries"] + 1 if edited else prev["tries"],
                "fp": keep_fp, "state": "",
                "hist": max(prev.get("hist", prev["best"]), pct)}
            if edited:
                stats["touched"] += 1
            if best - prev["best"] > EPS:
                stats["raised"] += 1
            elif prev["best"] - best > EPS:
                reset_by_edit.append((prev["best"] - best, key))

    for key, f in new_funcs.items():
        f["addr"] = rvas.get(key)

    # A missing current claim is still the same historical retail body: keep
    # its high-water row (state=absent) so a later natural emitter resumes
    # from the same MAX. Do NOT retain a row whose rva is currently claimed
    # (a rename/move handled above) or carved as library (a completed
    # reclassification - retaining it would pin a phantom).
    current_addrs = {addr for addr in rvas.values() if addr is not None}
    current_addrs |= library_rvas()
    dropped = []
    for key, old in base_funcs.items():
        if key in cur or key in migrated_old_keys:
            continue
        addr = old.get("addr")
        if addr is None or addr in current_addrs:
            dropped.append(key)
            continue
        row = dict(old)
        row["state"] = "absent"
        new_funcs[key] = row
        stats["preserved_absent"] += 1
    return new_funcs, stats, reset_by_edit, dropped


def _reconcile(overall, mods, started_fzw, started_code, eng, cur,
               base_funcs, rvas) -> None:
    old = rm.old_block_numbers(rm.current_block() or "")
    if old is None:
        return
    print("\n" + "=" * 74)
    print("RECONCILIATION: the README block was generated by the OLD pipeline;")
    print("the numbers below are recomputed by hobbit.verify. The new numbers")
    print("are the truth - the deltas are attributed, not bent to match.")
    print("=" * 74)
    new_exact = int(overall.get("matched_functions") or 0)
    moved = cl.migrations(cur, base_funcs, rvas)
    n_moved = sum(1 for o, n in moved.items() if o[0] != n[0])
    if "exact" in old:
        print(f"  exact functions:   {old['exact']:,} -> {new_exact:,} "
              f"({new_exact - old['exact']:+d})")
    lost100 = [(k, v) for k, v in base_funcs.items()
               if k not in cur and k not in moved
               and v.get("addr") is not None
               and v["best"] >= bl.EXACT]
    for k, v in lost100:
        print(f"      -1 attributed: {k[0]}/{k[1]} (banked exact at "
              f"0x{v['addr']:x}; its body is no longer emitted/scored - the "
              f"known full-tier finding; MAX preserved as state=absent)")
    for (u, fn), pct in sorted(cur.items()):
        if bl.EXACT <= pct < 100.0:
            print(f"      -1 attributed: {u}/{fn} scores {pct:.4f} - within "
                  f"the {bl.EXACT} EXACT slop the old pipeline counted, but "
                  f"below objdiff's ==100 matched_functions criterion")
    print(f"  unit re-homes:     {n_moved} bodies scored under a new unit "
          f"(rva-keyed join: same body, attribution only - COMDAT re-homing)")
    if "engine_fn" in old:
        print(f"  engine denominator: {old['engine_fn']:,} -> "
              f"{eng['real_fn']:,} ({eng['real_fn'] - old['engine_fn']:+d}); "
              f"classification authority moved from symbol_names/FID files to "
              f"the Model's winning channels, and the census grew. Carve-outs "
              f"old -> new:")
        for label, fn, code, _note in eng["categories"]:
            o = old.get(f"cat:{label}")
            if o:
                print(f"      {label}: {o[0]:,} fns / {o[1]:,} B -> "
                      f"{fn:,} fns / {code:,} B")
            else:
                print(f"      {label}: (not in old block) -> {fn:,} fns / "
                      f"{code:,} B")
    tot_code = eng["real_code"]
    new_fuzzy = started_fzw / tot_code if tot_code else 0.0
    if "fuzzy" in old:
        print(f"  overall fuzzy:     {old['fuzzy']:.2f}% -> {new_fuzzy:.2f}% "
              f"(denominator {old.get('engine_code', 0):,} -> {tot_code:,} "
              f"engine code B: the Model's derived extents span whole "
              f"contributions, trailing jump tables included)")
    md = scores.measure(overall, "matched_data_percent")
    if "matched_data_pct" in old and md is not None:
        print(f"  objdiff matched_data: {old['matched_data_pct']:.2f}% -> "
              f"{md:.2f}% (recovered DATA_COMPGEN identities move per-unit "
              f"data%)")
    print("  data coverage/fidelity table: the data-audit slice's product; "
          "not re-derived by verify - the new block carries objdiff's own "
          "matched_data only.")
    print("=" * 74)


def cmd_bank(argv) -> int:
    ap = argparse.ArgumentParser(prog="hobbit verify bank",
                                 description="update the baseline + README "
                                 "score block (a manual act)")
    ap.add_argument("--report", type=str, default=None,
                    help="an objdiff report.json (default: "
                         "build/objdiff/compare-new/report.json)")
    ap.add_argument("--dirty", action="store_true",
                    help="bank despite unstaged/untracked build inputs "
                         "(printed loudly)")
    ap.add_argument("--no-refresh", action="store_true",
                    help="skip the fingerprint-cache refresh")
    ap.add_argument("--baseline-only", action="store_true",
                    help="skip the README block refresh")
    a = ap.parse_args(argv)
    report = Path(a.report) if a.report else None

    require_bankable_tree("write config/match_baseline.tsv", a.dirty)
    if not a.no_refresh:
        from hobbit.verify.fingerprints import regenerate
        regenerate()

    doc, cur, base_funcs, fp, stale, rvas = load_state(report)
    _warn_stale_report(report)
    fails = scores.hard_failures(doc)
    if fails:
        for f in fails:
            print(f"HARD FAILURE: {f}", file=sys.stderr)
        raise SystemExit("refusing to bank a defective report")
    if stale:
        _warn_stale(stale)

    new_funcs, stats, reset_by_edit, dropped = \
        bank_rows(cur, base_funcs, fp, rvas)

    overall = doc.get("measures", {})
    umeas = scores.unit_measures(doc)
    mods, started_fzw, started_code = rm.collect_modules(umeas)
    from hobbit.model import resolve
    model = resolve()
    sizes = {(b.unit, b.name): b.size for b in model.functions
             if b.name and b.unit}
    from hobbit.verify.universe import engine_universe
    eng = engine_universe(model)

    if not a.baseline_only:
        _reconcile(overall, mods, started_fzw, started_code, eng, cur,
                   base_funcs, rvas)

    changed_b = bl.write(new_funcs)
    print(f"baseline {'UPDATED' if changed_b else 'unchanged'}: "
          f"{len(new_funcs)} functions across "
          f"{len({u for (u, _) in new_funcs})} units")
    print(f"  raised best: {stats['raised']}  tried(touched): "
          f"{stats['touched']}  new: {stats['added']}  moved(same rva): "
          f"{stats['moved']}  rebound(rva moved, best reset): "
          f"{stats['rebounds']}  preserved absent: "
          f"{stats['preserved_absent']}  dropped: {len(dropped)}")
    if reset_by_edit:
        reset_by_edit.sort(reverse=True)
        print(f"  best RESET by a source edit: {len(reset_by_edit)} "
              f"(hist_pct keeps the all-time peak):")
        for d, (u, fn) in reset_by_edit[:8]:
            print(f"    -{d:7.4f}  {u}  {fn}")
    for key in dropped[:8]:
        print(f"  dropped {key[0]}/{key[1]} (rva claimed elsewhere or "
              f"library-carved)")

    if not a.baseline_only:
        # `Fuzzy Max` reads the JUST-banked baseline, so the block and the
        # ledger describe the same tree state.
        banked = bl.load()
        target_doc, target_cur, sizes, other = rm.target_rollup(doc, model, rvas)
        mods, started_fzw, _ = rm.collect_modules(scores.unit_measures(target_doc))
        tot = rm.score_weights(target_cur, banked, sizes, mods, rm.unit_modules())
        eng["other_scored"] = other
        block = rm.render_block(mods, started_fzw, eng, tot)
        changed_r = rm.write_block(block)
        print(f"README score block "
              f"{'UPDATED' if changed_r else 'unchanged'} "
              f"({rm.README.relative_to(REPO)})")
        if not changed_b and not changed_r:
            print("bank: no-op (baseline and README already describe this "
                  "build)")
    elif not changed_b:
        print("bank: no-op (baseline already describes this build)")
    return 0
