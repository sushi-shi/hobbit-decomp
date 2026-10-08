"""hobbit - the umbrella CLI.

    hobbit tool <name> [args...]     drive one external tool (hobbit/tool/)
    hobbit labels [--all|--unit U]   source labels -> claim fragments (+ the
                                     tree-wide completeness sweep)
    hobbit model                     resolve claims x censuses -> bindings
    hobbit delink                    model -> synth pdb -> retail target objs
    hobbit compare                   base vs target -> objdiff report + summary
    hobbit build                     configure-if-needed + ninja (the loop:
                                     cl -> labels -> model -> delink -> compare)
    hobbit link                      the opt-in candidate link (EXE + .map)
    hobbit match                     build, then the compare summary for the
                                     units whose objs changed
    hobbit clean [--verify]          export a standalone C++ source project
    hobbit configure                 re-emit build/build.ninja
    hobbit sema <sub>                read-only investigation views (disasm,
                                     xref, rva, vtable, classof, strings, ...)
    hobbit walls <sub>               the wall campaign: inventory, diagnose,
                                     inline-model
    hobbit lineage <sub>             discover, inventory and verify surviving
                                     Entropy sibling-source correspondences
    hobbit permute <verb>            classified state/variant search or island campaign
    hobbit ghidra <sub>              one-way viewer export: the retail image
                                     as a labelled Ghidra project (build,
                                     update, verify, status, export)
    hobbit verify <sub>              status / check (the MAX gate) / bank
                                     (baseline + README, manual) /
                                     fingerprints
    hobbit rsrc check                compile Meridian.rc with era rc.exe,
                                     compare every resource with retail .rsrc
    hobbit lsp <verb>                clangd-backed refs / hover / rename (the
                                     type-aware bulk member renamer)
    hobbit init                      local setup (the build wine prefix; the
                                     dev-shell hook runs this at entry)
    hobbit inspect [--target NAME]    verify pinned inputs and show image facts

Subcommands grow with the rebuild; `tool` forwards to the named module's own
main(), so `hobbit tool cl ...` and `python3 -m hobbit.tool.cl ...` (the form
ninja rule lines use) are the same entry.
"""

from __future__ import annotations

import sys

TOOLS = ("wine", "cl", "link", "rc", "delinker", "pdbutil", "objdiff",
         "objdump", "ghidra", "clangd")


#: Query families where rc=1 answers "different", not "failed".
QUERY_COMMANDS = {"sema", "walls", "lineage"}


def main(argv: list[str] | None = None) -> int:
    """Run one command and record it in build/hobbit_usage.{log,jsonl}."""
    from hobbit.core.paths import BUILD
    from hobbit.core.usage import run_logged
    argv = list(sys.argv[1:] if argv is None else argv)
    failure_rc = 2 if argv and argv[0] in QUERY_COMMANDS else 1
    return run_logged(_dispatch, argv, BUILD / "hobbit_usage.log",
                      failure_rc=failure_rc)


def _dispatch(argv: list[str]) -> int:
    if not argv or argv[0] in ("-h", "--help"):
        print(__doc__.strip())
        print(f"\ntools: {', '.join(TOOLS)}")
        return 0 if argv else 2
    cmd, rest = argv[0], argv[1:]
    if cmd in ("labels", "model", "delink", "compare"):
        import importlib
        mod = importlib.import_module(
            {"labels": "hobbit.retail_labels.source", "model": "hobbit.model",
             "delink": "hobbit.delink.run", "compare": "hobbit.compare.run"}[cmd])
        sys.argv = [f"hobbit {cmd}", *rest]
        return mod.main()
    if cmd in ("sema", "walls", "ghidra", "verify", "rsrc", "lsp", "lineage", "clean"):
        import importlib
        return importlib.import_module(f"hobbit.{cmd}").main(rest)
    if cmd == "permute":
        if not rest or rest[0] in ("-h", "--help"):
            print("hobbit permute candidates [options]\n"
                  "hobbit permute campaign [--rva <rva>] [options]\n"
                  "hobbit permute state --source <tu.cpp> --rva <rva> [options]\n"
                  "hobbit permute variants <tu.cpp> <rva> [options]\n"
                  "  candidates: classify every live source-owned residual\n"
                  "  campaign: run N islands and retain M distinct best solutions\n"
                  "  state: classified, disposable compiler-state search\n"
                  "  variants: reviewed exact axes x AST shapes x TU state")
            return 0 if rest else 2
        if rest[0] in ("candidates", "campaign"):
            from hobbit.permute.campaign import main as campaign_main
            return campaign_main(rest)
        if rest[0] not in ("state", "variants"):
            print("hobbit permute: unknown verb " + repr(rest[0])
                  + " (have: candidates, campaign, state, variants)", file=sys.stderr)
            return 2
        verb, permute_args = rest[0], rest[1:]
        if any(value in ("-h", "--help") for value in permute_args):
            if verb == "state":
                from hobbit.permute.tu_state_noise import main as permute_main
            else:
                from hobbit.permute.match_variants import main as permute_main
            return permute_main(permute_args)
        rva_arg = (
            next((
                permute_args[index + 1]
                for index, value in enumerate(permute_args[:-1])
                if value == "--rva"
            ), None)
            if verb == "state"
            else (permute_args[1] if len(permute_args) >= 2 else None)
        )
        if rva_arg is None:
            print(f"hobbit permute {verb}: an RVA is required", file=sys.stderr)
            return 2
        from contextlib import redirect_stdout
        from io import StringIO
        from hobbit.walls.diagnose import diagnose
        diagnosis = StringIO()
        with redirect_stdout(diagnosis):
            result = diagnose(rva_arg)
        report = diagnosis.getvalue()
        print(report, end="")
        if result or "class: REGALLOC/SCHEDULING" not in report:
            print(f"hobbit permute {verb}: refused - permutation requires a "
                  "REGALLOC/SCHEDULING diagnosis", file=sys.stderr)
            return 2
        from hobbit.model import resolve
        from hobbit.verify.baseline import load as load_baseline
        rva = int(rva_arg, 0)
        from hobbit.core.pe import image
        if rva >= image().image_base:
            rva -= image().image_base
        binding = next((row for row in resolve().functions if row.rva == rva), None)
        bank = load_baseline().get((binding.unit, binding.name)) if binding else None
        if bank and bank["best"] >= 100.0:
            print(f"hobbit permute {verb}: refused - MAX is already 100%",
                  file=sys.stderr)
            return 2
        if verb == "state":
            from hobbit.permute.tu_state_noise import main as permute_main
        else:
            from hobbit.permute.match_variants import main as permute_main
        return permute_main(permute_args)
    if cmd in ("build", "link", "match"):
        from hobbit.graph.verbs import VERBS
        return VERBS[cmd](rest)
    if cmd == "configure":
        from hobbit.graph.emit import main as configure
        sys.argv = ["hobbit configure", *rest]
        return configure()
    if cmd == "init":
        import argparse
        from pathlib import Path
        from hobbit.core.inputs import InputError, targets, stage_executable
        parser = argparse.ArgumentParser(prog="hobbit init")
        parser.add_argument("--exe", type=Path)
        parser.add_argument("--xbox", type=Path)
        parser.add_argument("--xbox-map", type=Path)
        options = parser.parse_args(rest)
        from hobbit.tool import ToolError
        from hobbit.tool.wine import init_prefix, verify_prefix
        try:
            pins = targets()
            stage_executable(pins["game"], options.exe)
            for key in ("xbox", "xbox_map"):
                supplied = getattr(options, key)
                if supplied is not None or pins[key].destination.exists():
                    stage_executable(pins[key], supplied)
            init_prefix()
            verify_prefix()
        except (ToolError, InputError) as e:
            print(f"[init] {e}", file=sys.stderr)
            return 1
        print("[init] build wine prefix OK (the graph/init steps grow with "
              "the rebuild)")
        return 0
    if cmd == "inspect":
        import argparse
        import json
        from hobbit.core.inputs import targets, read_verified
        from hobbit.core.pe import Pe
        from hobbit.core.paths import retail_exe
        pins = targets()
        parser = argparse.ArgumentParser(prog="hobbit inspect")
        parser.add_argument("--target", choices=sorted(pins), default="game")
        args = parser.parse_args(rest)
        pin = pins[args.target]
        path = retail_exe() if args.target == "game" else pin.destination
        read_verified(pin, path)
        result = {"target": args.target, "path": str(path),
                  "size": pin.size, "sha256": pin.sha256}
        if args.target == "game":
            pe = Pe(path)
            result.update(image_base=pe.image_base, sections=pe.sections)
        print(json.dumps(result, indent=2))
        return 0
    if cmd == "tool":
        if not rest or rest[0] not in TOOLS:
            print(f"hobbit tool: pick one of {', '.join(TOOLS)}", file=sys.stderr)
            return 2
        import importlib
        mod = importlib.import_module(f"hobbit.tool.{rest[0]}")
        sys.argv = [f"hobbit tool {rest[0]}", *rest[1:]]
        return mod.main()
    print(f"hobbit: unknown command {cmd!r} (the rebuild grows these "
          "step by step; see scripts/hobbit/__init__.py)", file=sys.stderr)
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
