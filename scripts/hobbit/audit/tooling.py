"""Inventory the pinned local Gruntz tooling and supporting workflow files.

Adapted from local HoMM1 scripts/homm1/audit/tooling.py at
8d3ae6c96fc9b06d9c709fbdbfa181d78997b787. Only Gruntz is inventoried as the
package donor. Hash/AST comparisons establish structure, never behavioral
parity, target correctness, or successful execution of dormant commands.
"""

from __future__ import annotations

import argparse
import ast
from collections import Counter
import hashlib
import json
from pathlib import Path
import subprocess

from hobbit.core.paths import REPO

REVISION = "7d4bd55b99e32f084834d991badf7609889481f4"
DONOR = Path("/home/sheep/Projects/gruntz")
PREFIX = "scripts/gruntz/"
SUPPORT = ("editor/", "nix/", ".agents/")
SUPPORT_FILES = {".clangd", ".clang-format", ".githooks/pre-commit", "AGENTS.md",
                 "config/units.toml", "include/rva.h", "flake.nix", "flake.lock"}
OMISSIONS = {
    "retail_labels/test_message_maps.py": "Donor SDK integration fixtures require Gruntz StdAfx.h, CDialog, MFC annotation macros and AFXMSG_.H mirror patch absent in Hobbit. Retained generic parser is not validated by these inapplicable fixtures.",
    "tool/rez.py": "Monolith REZ is a Gruntz archive format, not Hobbit DFS; no fabricated Hobbit.REZ capability.",
    "graph/play.py": "Gruntz launcher/runtime deployment is game-specific; this task does not launch the game.",
    "graph/test_play.py": "Tests the omitted Gruntz game launcher.",
    "clean/project/play.py": "Standalone Gruntz game launcher is not a reconstructed Hobbit executable.",
    "clean/test_play.py": "Tests the omitted standalone Gruntz game launcher.",
    "clean/project/imports/mss32.c": "Donor hardcoded import stub replaced by Hobbit PE import recovery; not copied into source export.",
    "clean/project/imports/smackw32.c": "Gruntz Smacker import stub is not Hobbit Bink import evidence.",
}


def _git(donor: Path, *args: str) -> bytes:
    return subprocess.check_output(["git", "-C", str(donor), *args])


def normalized_ast(text: str) -> str:
    """Ignore namespace renaming, comments/docstrings and formatting only."""
    text = text.replace("gruntz", "hobbit").replace("GRUNTZ", "HOBBIT")
    tree = ast.parse(text)
    for node in ast.walk(tree):
        if isinstance(node, (ast.Module, ast.ClassDef, ast.FunctionDef,
                             ast.AsyncFunctionDef)) and node.body:
            first = node.body[0]
            if (isinstance(first, ast.Expr) and isinstance(first.value, ast.Constant)
                    and isinstance(first.value.value, str)):
                node.body = node.body[1:]
    return ast.dump(tree, include_attributes=False)


def inventory(donor: Path) -> dict:
    paths = _git(donor, "ls-tree", "-r", "--name-only", REVISION).decode().splitlines()
    selected = [p for p in paths if p.startswith((PREFIX, *SUPPORT)) or p in SUPPORT_FILES]
    if not any(p.startswith(PREFIX) for p in selected):
        raise ValueError("pinned Gruntz tree contains no package files")
    rows = []
    for path in selected:
        original = _git(donor, "show", f"{REVISION}:{path}")
        relative = path.removeprefix(PREFIX)
        destination = path.replace("scripts/gruntz/", "scripts/hobbit/")
        destination = destination.replace("gruntzbuild/", "hobbitbuild/")
        destination = destination.replace("/gruntz/", "/hobbit/").replace("/gruntz.lua", "/hobbit.lua")
        row = {"path": path, "scope": "package" if path.startswith(PREFIX) else "support",
               "donor_sha256": hashlib.sha256(original).hexdigest(), "destination": destination}
        target = REPO / destination
        if path.startswith(PREFIX) and relative in OMISSIONS and not target.exists():
            row.update(status="inapplicable", reason=OMISSIONS[relative])
        elif path == "nix/runtime.nix" and not target.exists():
            row.update(status="inapplicable", reason="Donor installed-game runtime/launcher environment; build Wine support is in Hobbit toolchain and tool/wine.py.")
        elif path.startswith(".agents/"):
            row.update(status="reference_only", reason="Local donor skill remains readable; do not import foreign game instructions or override read-only project agent configuration.")
        elif path == ".githooks/pre-commit" and not target.exists():
            row.update(status="not_installed_optional", reason="Donor hook reformats and restages whole files. No Git hook configuration or automatic restaging authorized; formatter configuration is available separately.")
        elif not target.is_file():
            row.update(status="missing", reason="No local file or recorded replacement.")
        else:
            current = target.read_bytes()
            row["local_sha256"] = hashlib.sha256(current).hexdigest()
            if current == original:
                row["status"] = "identical_bytes"
            elif path.endswith(".py"):
                try:
                    same = normalized_ast(original.decode()) == normalized_ast(current.decode())
                    row["status"] = "equivalent_ast_after_rename" if same else "adapted_review_required"
                except (SyntaxError, UnicodeError) as error:
                    row.update(status="parse_error", reason=str(error))
            else:
                row["status"] = "adapted_review_required"
        rows.append(row)
    package = [r for r in rows if r["scope"] == "package"]
    return {"repository": str(donor), "revision": REVISION,
            "audit_donor": "HoMM1 scripts/homm1/audit/tooling.py",
            "audit_donor_revision": "8d3ae6c96fc9b06d9c709fbdbfa181d78997b787",
            "scope": "All pinned Gruntz package files, plus editor, Nix, skills, annotation header, build manifest and selected root workflow files.",
            "limitation": "Structural inventory only. Presence or normalized AST equality does not establish working behavior or target parity.",
            "package_file_count": len(package), "support_file_count": len(rows) - len(package),
            "counts": dict(sorted(Counter(r["status"] for r in rows).items())), "files": rows}


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--donor", type=Path, default=DONOR)
    parser.add_argument("--json", action="store_true", help="Print complete inventory")
    parser.add_argument("--write", type=Path, help="Explicitly write inventory snapshot")
    args = parser.parse_args(argv)
    report = inventory(args.donor)
    payload = json.dumps(report, indent=2) + "\n"
    if args.write:
        args.write.parent.mkdir(parents=True, exist_ok=True)
        args.write.write_text(payload)
    if args.json:
        print(payload, end="")
    else:
        print(f"Gruntz {REVISION}: {report['package_file_count']} package files, "
              f"{report['support_file_count']} support files")
        for status, count in report["counts"].items():
            print(f"  {status}: {count}")
        for row in report["files"]:
            if row["status"] in ("missing", "parse_error"):
                print(f"  {row['status'].upper()}: {row['path']}")
        print(report["limitation"])
    return int(any(r["status"] in ("missing", "parse_error") for r in report["files"]))


if __name__ == "__main__":
    raise SystemExit(main())
