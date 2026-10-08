"""hobbit.graph.verbs - the three verbs that drive the graph.

    hobbit build [targets...] [-j N] [--force-delink] [-v]
    hobbit link  [--engine-lib] [--order F] ...     -> `ninja candidate`
    hobbit match <unit|source>...                   -> the fast loop: compile,
                                                       label, delink, compare
                                                       only those units
    hobbit match [--reference R] [--all]            -> build, then the deltas
    hobbit play  [--retail]                         -> build + link, install
                                                       into the game env, run
                                                       scaled (play.sh)

`build` configures if the manifest is missing (after that ninja's generator
edge owns it) and runs the default target. `link` is the same graph with the
opt-in phase-2 target. `match <unit>` is the matching loop: it rebuilds only the
named units (an edited header is not propagated to other TUs), re-delinks only
when their labels changed, and prints their functions against the banked MAX;
everything else keeps its last build until `hobbit build`. Bare `match` builds
the compare target and reports the units whose objects CHANGED. Gates run only
for `hobbit build verify`, when preparing a merge.

"Changed" is decided by CONTENT, not mtime: hobbit.graph.cc writes objects
if-changed with the COFF timestamp stabilised, so a hash census before and
after the build names precisely the units whose codegen moved.
"""

from __future__ import annotations

import hashlib
import subprocess
import sys
from pathlib import Path

from hobbit import graph
from hobbit.core.paths import REPO


def toolchain_repinned() -> bool:
    """True when $MSVC_DIR/$DXSDK_DIR/the delinker no longer match the manifest.

    The generator edge cannot answer this: ninja reruns it from FILES, and the
    toolchain is environment. So the driver checks it before handing over. A
    re-pin must reconfigure rather than merely rebuild, because whether the
    `rc` edge exists at all is decided at configure time - that is how a
    pre-r3 shell silently produced a candidate with no `.rsrc`.
    """
    from hobbit.graph.emit import toolchain_id
    path = REPO / graph.TOOLCHAIN_ID
    if not path.exists():
        return (REPO / graph.NINJA).exists()
    return path.read_text() != toolchain_id()


def configure_if_needed(force: bool = False) -> None:
    """Emit build/build.ninja when it is absent, forced, or the toolchain moved."""
    repinned = toolchain_repinned()
    if force or repinned or not (REPO / graph.NINJA).exists():
        if repinned and not force:
            print("[configure] the pinned toolchain or delinker moved since this "
                  "manifest was written - reconfiguring", file=sys.stderr)
        from hobbit.graph.emit import emit
        n, pruned = emit()
        print(f"[configure] wrote {graph.NINJA} ({n} units"
              + (f", pruned {pruned} orphan artifact(s)" if pruned else "") + ")")


def ninja(targets: list[str] = (), *, jobs: int | None = None,
          verbose: bool = False, keep_going: bool = False,
          extra: list[str] = ()) -> int:
    """Run ninja against the emitted manifest from the repo root."""
    argv = ["ninja", "-f", graph.NINJA]
    if jobs:
        argv += ["-j", str(jobs)]
    if verbose:
        argv.append("-v")
    if keep_going:
        argv += ["-k", "0"]
    argv += [*extra, *targets]
    from hobbit.core.usage import run_process
    return run_process(argv, cwd=REPO)


def object_census() -> dict[str, str]:
    """{unit: sha256} over the base objects present right now."""
    base = REPO / graph.BASE_DIR
    if not base.is_dir():
        return {}
    return {p.stem: hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(base.glob("*.obj"))}


def changed_units(before: dict[str, str], after: dict[str, str]) -> list[str]:
    return sorted(u for u in after if before.get(u) != after[u])


# --------------------------------------------------------------------------- #
# hobbit build
# --------------------------------------------------------------------------- #
def build_main(argv: list[str] | None = None) -> int:
    """Configure-if-needed, then ninja's default target."""
    import argparse
    ap = argparse.ArgumentParser(prog="hobbit build", description=build_main.__doc__)
    ap.add_argument("targets", nargs="*", help="ninja targets (default: all)")
    ap.add_argument("-j", "--jobs", type=int)
    ap.add_argument("-v", "--verbose", action="store_true")
    ap.add_argument("--reconfigure", action="store_true",
                    help="re-emit build/build.ninja before building")
    ap.add_argument("--force-delink", action="store_true",
                    help="drop the delink stamp so the delinker re-runs even "
                         "though bindings.tsv did not change")
    a = ap.parse_args(argv)
    configure_if_needed(a.reconfigure)
    if a.force_delink:
        (REPO / graph.DELINK_STAMP).unlink(missing_ok=True)
    return ninja(a.targets, jobs=a.jobs, verbose=a.verbose)


# --------------------------------------------------------------------------- #
# hobbit link
# --------------------------------------------------------------------------- #
def manifest_targets() -> set[str]:
    """Every output the emitted manifest declares an edge for.

    Asked of the MANIFEST, not of `era_rc_available()`: the `.res` edge exists
    only when the toolchain carried rc.exe at CONFIGURE time, and $MSVC_DIR is
    not a declared input, so the emitter's answer and the file on disk can
    disagree. Requesting a target the manifest does not have is a hard
    `ninja: error: unknown target`, which is how `hobbit link --anything`
    (`--help` included) used to die on a pre-r3 toolchain.
    """
    out: set[str] = set()
    try:
        text = (REPO / graph.NINJA).read_text(encoding="utf-8")
    except OSError:
        return out
    for line in text.replace("$\n", " ").splitlines():
        if line.startswith("build "):
            out.update(line[len("build "):].split(":", 1)[0].split())
    return out


def link_main(argv: list[str] | None = None) -> int:
    """Build the opt-in candidate image + .map (`ninja candidate`).

    Bare `hobbit link` goes through the graph, so the objects it links are the
    ones the current sources produce. Any hobbit.graph.link option (--order,
    --engine-lib, --incremental, ...) switches to a direct link of whatever
    is in build/objdiff/base, since those are experiments on a fixed object set.
    `--help` is answered by that parser without building anything.
    """
    argv = list(sys.argv[1:] if argv is None else argv)
    from hobbit.graph.link import main as link_direct
    if any(a in ("-h", "--help") for a in argv):
        sys.argv = ["hobbit link", *argv]
        return link_direct()
    configure_if_needed()
    if not argv:
        if "candidate" not in manifest_targets():
            print("[link] candidate requires [link].enabled and an evidence-backed entrypoint/library policy in config/units.toml; the partial reconstruction can already match individual objects.", file=sys.stderr)
            return 1
        return ninja(["candidate"])
    targets = ["base"]
    if graph.RESOURCE_RES in manifest_targets():
        targets.append(graph.RESOURCE_RES)
    elif not any(a == "--res" or a.startswith("--res=") for a in argv):
        print("[link] no reconstructed resource script: candidate map study omits .rsrc", file=sys.stderr)
    rc = ninja(targets)
    if rc:
        return rc
    sys.argv = ["hobbit link", *argv]
    return link_direct()


# --------------------------------------------------------------------------- #
# hobbit match
# --------------------------------------------------------------------------- #
def _pct(measures: dict, key: str = "fuzzy_match_percent") -> float:
    return float(measures.get(key) or 0.0)


def print_changed(report: dict, units: list[str], *, functions: bool = True,
                  limit: int = 60) -> None:
    """The compare summary restricted to `units`, plus the overall line."""
    by_name = {u["name"]: u for u in report.get("units", [])}
    print(f"\n{'unit':<32} {'fuzzy%':>8} {'fns':>6} {'matched':>8} {'code':>9}")
    print("-" * 68)
    for name in sorted(units, key=lambda n: (_pct(by_name.get(n, {}).get(
            "measures", {})), n)):
        u = by_name.get(name)
        if u is None:
            print(f"{name:<32} {'(not in the report - no pairing)':>35}")
            continue
        m = u["measures"]
        print(f"{name:<32} {_pct(m):>8.2f} {m.get('total_functions', 0):>6} "
              f"{m.get('matched_functions', 0):>8} {m.get('total_code', 0):>9}")
    if functions:
        rows = [(n, fn) for n in units for fn in by_name.get(n, {}).get("functions", [])
                if _pct(fn) < 100.0]
        if rows:
            print(f"\n--- functions below 100% in the {len(units)} changed unit(s) "
                  f"({len(rows)}) ---")
            print(f"{'unit':<28} {'symbol':<58} {'fuzzy%':>8}")
            for name, fn in sorted(rows, key=lambda r: _pct(r[1]))[:limit]:
                print(f"{name:<28} {fn.get('name', ''):<58} {_pct(fn):>8.2f}")
            if len(rows) > limit:
                print(f"... and {len(rows) - limit} more")
    m = report.get("measures", {})
    print("-" * 68)
    print(f"overall fuzzy {_pct(m):.5f}%  "
          f"functions {m.get('matched_functions', 0)}/{m.get('total_functions', 0)} "
          f"({_pct(m, 'matched_functions_percent'):.2f}%)  "
          f"code {m.get('matched_code', 0)}/{m.get('total_code', 0)} "
          f"({_pct(m, 'matched_code_percent'):.2f}%)  "
          f"units {m.get('total_units', 0)}")


def resolve_units(specs: list[str]) -> list[str]:
    """Unit stems from stems or source paths (`fader`, `src/DDrawMgr/Fader.cpp`)."""
    from hobbit.manifest import units as manifest_units
    rows = manifest_units()
    by_stem = {u["unit"]: u for u in rows}
    by_source = {str(Path(u["source"])): u["unit"] for u in rows}
    out = []
    for spec in specs:
        if spec in by_stem:
            out.append(spec)
            continue
        path = Path(spec)
        if path.is_absolute():
            try:
                path = path.resolve().relative_to(REPO)
            except ValueError:
                pass
        unit = by_source.get(str(path))
        if unit is None:
            raise SystemExit(f"hobbit match: {spec!r} is not a unit stem or a "
                             "unit source in config/units.toml")
        out.append(unit)
    return list(dict.fromkeys(out))


def match_units(units: list[str], *, jobs: int | None, verbose: bool) -> int:
    """The fast loop: compile, label, delink, and compare only `units`.

    Every other unit keeps its last-built objects, claims, and scores, even
    when an edited header would change them; `hobbit build` refreshes them.
    No fingerprints, no gates.
    """
    import time

    from hobbit.compare import normalize, project
    from hobbit.delink import run as delink
    from hobbit.manifest import units as manifest_units
    from hobbit.model import resolve, serialize
    from hobbit.tool import objdiff
    from hobbit.verify import scores

    started = time.monotonic()
    report_path = REPO / graph.REPORT_JSON

    targets = [f"{graph.BASE_DIR}/{u}.obj" for u in units]
    targets += [f"{graph.CLAIMS_DIR}/{u}.tsv" for u in units]
    rc = ninja(targets, jobs=jobs, verbose=verbose)
    if rc:
        return rc

    model = resolve()
    bindings_changed, _ = serialize(model)
    target_dir = REPO / graph.TARGET_DIR
    missing = [u for u in units if not (target_dir / f"{u}.c.obj").exists()]
    if bindings_changed or missing:
        delink.run(model, target_dir=target_dir, only=units)
        project.project(manifest_units(), target_dir, REPO / graph.COMPARE_DIR)
    normalize.normalize(REPO / graph.BASE_DIR, target_dir,
                        REPO / graph.COMPARE_DIR, units)
    objdiff.report(REPO / graph.COMPARE_DIR, report_path)

    after = scores.functions(scores.load(report_path))
    print_unit_functions(units, after)
    print(f"\n[match] {', '.join(units)} in {time.monotonic() - started:.1f}s"
          + (" (labels changed: delinked)" if bindings_changed else ""))
    return 0


def print_unit_functions(units: list[str], after: dict) -> None:
    """MAX movement only. An unchanged function keeps its banked MAX, so a CUR
    dip is noise and stays silent; it is listed only when it rises above MAX.
    An edited function's MAX becomes its new score, so it is listed with the
    MAX it replaces: `drop` when the edit moved its CUR down, `reset` when CUR
    held and only the new source hash lowered MAX (HIST keeps the old peak)."""
    from contextlib import redirect_stdout
    from io import StringIO

    from hobbit.verify import baseline
    from hobbit.verify.baseline import EPS
    from hobbit.verify.fingerprints import fingerprinter, real_edit, regenerate
    with redirect_stdout(StringIO()):
        regenerate()
    fp, _cpp_of, _stale = fingerprinter()
    bank = baseline.load()
    resets = []
    for unit in units:
        rows = sorted((name, pct) for (u, name), pct in after.items() if u == unit)
        if not rows:
            print(f"\n{unit}: no paired functions in the report")
            continue
        shown, at_max = [], 0
        for name, pct in rows:
            row = bank.get((unit, name))
            if row is None:
                shown.append((pct, None, name, "new"))
                at_max += pct >= 100.0
                continue
            edited = real_edit(row["fp"], fp(unit, name))
            new_max = pct if edited else max(row["best"], pct)
            at_max += new_max >= 100.0
            if edited and pct > row["best"] + EPS:
                shown.append((pct, row["best"], name, "up"))
            elif edited and row["best"] - pct > EPS:
                # CUR held against the bank, as the MAX gate classifies it: the
                # worktree's previous report may already contain the edit.
                held = abs(pct - row["cur"]) <= EPS
                if held:
                    resets.append((row.get("addr"), unit, name, row["best"], pct))
                shown.append((pct, row["best"], name,
                              "reset: CUR held, recorded for syntactic recovery"
                              if held else "drop"))
            elif edited and pct < 100.0:
                shown.append((pct, row["best"], name, "edited"))
            elif not edited and pct > row["best"] + EPS:
                shown.append((pct, row["best"], name, "up"))
        print(f"\n{unit}: {at_max}/{len(rows)} at MAX 100")
        if not shown:
            print("  no MAX change")
            continue
        print(f"  {'now':>8} {'max':>8}  function  [kind]")
        for pct, best, name, kind in sorted(shown, key=lambda r: (r[0], r[2])):
            print(f"  {pct:8.2f} {'' if best is None else f'{best:8.2f}':>8}  "
                  f"{name}  [{kind}]")
    record_resets(resets)


#: Functions whose MAX an edit lowered while their CUR held: a later
#: fuzzy syntactic recovery pass looks for a spelling that regains the peak.
RECOVERY_TODO = REPO / "docs/todos/syntactic-recovery.tsv"


def record_resets(resets: list) -> None:
    """Add or update one row per reset function, keeping the highest lost MAX."""
    if not resets:
        return
    header = "rva\tunit\tfunction\tlost_max\tcur\n"
    rows: dict[tuple[str, str], list[str]] = {}
    if RECOVERY_TODO.exists():
        for line in RECOVERY_TODO.read_text().splitlines()[1:]:
            cols = line.split("\t")
            if len(cols) == 5:
                rows[(cols[1], cols[2])] = cols
    for addr, unit, name, lost_max, pct in resets:
        old = rows.get((unit, name))
        peak = max(lost_max, float(old[3])) if old else lost_max
        rows[(unit, name)] = ["" if addr is None else f"0x{addr:06x}", unit,
                              name, f"{peak:.4f}", f"{pct:.4f}"]
    text = header + "".join("\t".join(r) + "\n" for r in sorted(
        rows.values(), key=lambda r: (r[1], r[2])))
    if not RECOVERY_TODO.exists() or RECOVERY_TODO.read_text() != text:
        RECOVERY_TODO.parent.mkdir(parents=True, exist_ok=True)
        RECOVERY_TODO.write_text(text)


def match_main(argv: list[str] | None = None) -> int:
    """Fast loop for named units; with none, build everything and summarise
    the units whose objects changed."""
    import argparse
    ap = argparse.ArgumentParser(prog="hobbit match", description=match_main.__doc__)
    ap.add_argument("units", nargs="*",
                    help="unit stems or source paths: compile, delink and "
                         "compare only these")
    ap.add_argument("-j", "--jobs", type=int)
    ap.add_argument("-v", "--verbose", action="store_true")
    ap.add_argument("--reference", type=Path,
                    help="an earlier report.json to diff per-function scores against")
    ap.add_argument("--all", action="store_true",
                    help="summarise every unit below 100%%, not only the changed ones")
    ap.add_argument("--no-functions", dest="functions", action="store_false",
                    help="unit rows only")
    ap.add_argument("-k", "--keep-going", action="store_true",
                    help="build every edge that can still build, then summarise "
                         "anyway. The exit code still reports the failure and "
                         "the summary is flagged as possibly stale - use it "
                         "when one broken edge would otherwise hide 310 units")
    a = ap.parse_args(argv)

    configure_if_needed()
    if a.units:
        return match_units(resolve_units(a.units), jobs=a.jobs, verbose=a.verbose)

    from hobbit.verify import scores
    before = object_census()
    rc = ninja(["compare"], jobs=a.jobs, verbose=a.verbose,
               keep_going=a.keep_going)
    if rc and not a.keep_going:
        return rc
    after = object_census()
    changed = changed_units(before, after)

    from hobbit.compare.run import print_reference_diff, print_summary
    from hobbit.tool import objdiff
    report_path = REPO / graph.REPORT_JSON
    if not report_path.exists():
        print(f"[match] no report at {graph.REPORT_JSON} - the build produced "
              "none", file=sys.stderr)
        return 1
    report = objdiff.load(report_path)

    if rc:
        print(f"\n[match] BUILD FAILED (ninja rc={rc}) - the summary below is "
              "whatever the last complete report says and may be STALE",
              file=sys.stderr)
    print(f"\n[match] {len(changed)} unit object(s) changed"
          + (f": {', '.join(changed[:12])}" + (" ..." if len(changed) > 12 else "")
             if changed else " (nothing rebuilt)"))
    if a.all or not changed:
        print_summary(report, all_units=False)
    elif a.functions:
        print_unit_functions(changed,
                             scores.functions(scores.load(report_path)))
    else:
        print_changed(report, changed, functions=False)
    if a.reference is not None:
        try:
            reference = objdiff.load(a.reference)
        except (OSError, ValueError) as e:
            print(f"[match] --reference {a.reference} is not a readable "
                  f"objdiff report: {e}", file=sys.stderr)
            return rc or 2
        print_reference_diff(reference, report)
    return rc


VERBS = {"build": build_main, "link": link_main, "match": match_main}


def main() -> int:
    argv = sys.argv[1:]
    if not argv or argv[0] not in VERBS:
        print(f"usage: python3 -m hobbit.graph.verbs {{{'|'.join(VERBS)}}} [args]",
              file=sys.stderr)
        return 2
    verb, rest = argv[0], argv[1:]
    if verb == "link":
        return link_main(rest)
    return VERBS[verb](rest)


if __name__ == "__main__":
    raise SystemExit(main())
