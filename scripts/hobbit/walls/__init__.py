"""hobbit.walls - the wall-breaking slice: the remaining matching campaign.

    hobbit walls inventory        the derived worklist (report x Model x
                                  match_baseline) - ascending historical MAX
    hobbit walls abstractions     classify every sub-100 row by the semantic
                                  level to inspect before source-shape work
    hobbit walls diagnose <fn>    classify one wall from the normalized pair:
                                  referent -> inline/call-set -> cfg -> regalloc
    hobbit walls inline-model     the cl 5.0 inline-budget model. `--gap <rva>`
                                  names the call-set delta and reports the base
                                  obj's candidacy evidence. A defined COMDAT
                                  proves inline visibility; an undefined or
                                  absent symbol is ambiguous and needs source,
                                  /Ob0, or call-site-topology evidence.
                                  `--gap <spec.json>` quantifies the deficit
                                  once cb is known; --measure-cb titrates cb
                                  with the real compiler
    hobbit walls semdiff <fn>     OPERAND-LEVEL adjudication of one pair:
                                  exclusive fp/disp/store/imm keys, plus the
                                  ordered referent sequence a masked diff
                                  structurally cannot show
    hobbit walls semsweep <tsv>   the same screen over a worklist range
    hobbit walls switchmap <fn>   switch jump tables: the cases whose arms
                                  reach different constants, referents or
                                  callees (--all: every sub-100 row)
    hobbit walls aggregate-copies rep-movs count sieve; a source/CFG lead,
                                  never proof until block merging is excluded
    hobbit walls aggdecl          aggregate-vs-scalar declaration sieve per
                                  member pair (--control re-proves it)
    hobbit walls aggscan          by-value aggregate argument sieve, keyed on
                                  the callee over the whole image
    hobbit walls valuetemp        by-value struct temp sieve: an inlined
                                  accessor's dead half-store in the frame
    hobbit walls eh-frame         /GX frame-presence + unwind-state sieve,
                                  cause-tagged (inline/merge/state-flow/object)
    hobbit walls global-refs      global read-COUNT sieve (the cached-global
                                  bug class; --calibrate = detector-bug rate)
    hobbit walls stale-markers    @early-stop markers sitting on 100% bodies
    hobbit walls review           source-hash-scoped reviewer ledger
    hobbit walls recheck          re-measure the counts a review certifies
                                  against today's pair
    hobbit walls priors           both prior-verdict stores for a worklist -
                                  the comment above the RVA() pin AND the
                                  review ledger row - screened before any A/B

The worklist is derived from the compare report every time. The review
ledger records reviewer progress and invalidates each row when its source
hash changes; it is not evidence that a reconstruction is correct.

Input surface: the Model, the compare out-dir (report.json + normalized
objs), config/match_baseline.tsv, tool.objdump/tool.cl, delink.coffx (the
shared COFF topology reader). Read-only except inline-model's scratch
harness compiles under build/inline-model/.
"""

from __future__ import annotations

_SUBS = {"inventory": "hobbit.walls.inventory",
         "abstractions": "hobbit.walls.abstractions",
         "diagnose": "hobbit.walls.diagnose",
         "inline-model": "hobbit.walls.inline_model",
         "aggregate-copies": "hobbit.walls.aggregate_copies",
         "eh-frame": "hobbit.walls.eh_frame",
         "aggscan": "hobbit.walls.aggscan",
         "aggdecl": "hobbit.walls.aggdecl",
         "valuetemp": "hobbit.walls.valuetemp",
         "global-refs": "hobbit.walls.global_refs",
         "semdiff": "hobbit.walls.semdiff",
         "semsweep": "hobbit.walls.semdiff",
         "switchmap": "hobbit.walls.switchmap",
         "stale-markers": "hobbit.walls.stale_markers",
         "priors": "hobbit.walls.priors",
         "recheck": "hobbit.walls.recheck",
         "review": "hobbit.walls.reviews"}


def check_unit(unit: str | None) -> str | None:
    """`--unit` filters answer 0/none for a name nobody has - which reads as a
    clean result rather than a typo. Reject an unknown unit here instead."""
    if unit is None:
        return None
    from hobbit.manifest import units as manifest_units
    known = {u["unit"] for u in manifest_units()}
    if unit in known:
        return unit
    import difflib
    import sys
    near = difflib.get_close_matches(unit, sorted(known), n=3)
    print(f"[walls] unknown unit {unit!r} - not in config/units.toml"
          + (f" (did you mean: {', '.join(near)}?)" if near else "")
          + "\n        `hobbit sema map units` lists the units that claim rows",
          file=sys.stderr)
    raise SystemExit(2)


def main(argv=None) -> int:
    import importlib
    import sys
    argv = list(sys.argv[1:] if argv is None else argv)
    if not argv or argv[0] in ("-h", "--help"):
        print(__doc__.strip())
        return 0 if argv else 2
    if argv[0] not in _SUBS:
        print(f"hobbit walls: unknown verb {argv[0]!r} (have: "
              f"{', '.join(_SUBS)})", file=sys.stderr)
        return 2
    mod = importlib.import_module(_SUBS[argv[0]])
    sys.argv = [f"hobbit walls {argv[0]}", *argv[1:]]
    entry = mod.sweep_main if argv[0] == "semsweep" else mod.main
    return entry(argv[1:])
