"""Clang compilation database for navigation and source annotation extraction.

Adapted from local Gruntz graph/compdb.py. The era compiler builds the matching
objects; Clang parses with the same unit defines, includes, EH and RTTI profile.
VC6 compatibility is a Hobbit toolchain hypothesis, recorded in units.toml.
"""

from __future__ import annotations

import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

from hobbit.core.paths import BUILD, INCLUDE, REPO, VENDOR, dxsdk_dir, msvc_dir

OUT_DIR = BUILD / "clangd"
OUT_FILE = OUT_DIR / "compile_commands.json"
MIRROR_DIR = OUT_DIR / "inc-lower"

#: Default parser compatibility for the tentative VC6 matching compiler.
MSC_COMPAT = "12.00"
TARGET = "i386-pc-windows-msvc"

# Extra defines belong to the complete per-unit flag profile.
DEFINES = []


def resolve_include_dirs() -> tuple[Path, Path | None, str]:
    """(msvc_include, dx_include, provenance) for the era toolchain headers.

    Prefers the dev shell's $MSVC_DIR/$DXSDK_DIR; otherwise builds
    .#hobbit-toolchain and reads the include dirs off the store path.
    """
    try:
        msvc_inc = msvc_dir() / "include"
        try:
            dx_inc = dxsdk_dir() / "Include"
            if not dx_inc.is_dir():
                raise SystemExit(f"[compdb] configured SDK include directory missing: {dx_inc}")
        except RuntimeError:
            dx_inc = None
        if msvc_inc.is_dir():
            return msvc_inc, dx_inc, "env ($MSVC_DIR; optional $DXSDK_DIR)"
    except RuntimeError:
        pass
    print("[compdb] MSVC_DIR not in env - "
          "running `nix build .#hobbit-toolchain` ...", file=sys.stderr)
    try:
        out = subprocess.check_output(
            ["nix", "build", ".#hobbit-toolchain", "--no-link",
             "--print-out-paths"], cwd=str(REPO), text=True,
        ).strip().splitlines()
    except (subprocess.CalledProcessError, FileNotFoundError) as e:
        raise SystemExit(
            "[compdb] ERROR: could not resolve the toolchain headers. Run "
            "inside `nix develop` (sets MSVC_DIR), or ensure "
            f"`nix build .#hobbit-toolchain` works.\n  cause: {e}") from e
    root = Path(out[-1])
    msvc_inc, dx_inc = root / "msvc" / "include", root / "dx" / "Include"
    if not msvc_inc.is_dir():
        raise SystemExit(f"[compdb] ERROR: toolchain at {root} is missing "
                         f"msvc/include.")
    return msvc_inc, dx_inc if dx_inc.is_dir() else None, f"nix build .#hobbit-toolchain ({root})"


def normalize_era_utility(mirror: Path) -> None:
    """Clang-only spelling repair: VC6 UTILITY repeats IOSFWD defaults.

    Remove just those two redundant defaults on the iterator definitions.
    The declared types and genuine bodies are unchanged; CL uses the original
    SDK include tree, never this generated Linux case-lookup mirror.
    """
    path = mirror / "utility"
    if not path.is_file():
        return
    raw = path.read_bytes()
    old = b"template<class _E, class _Tr = char_traits<_E> >\r\n\tclass "
    # Genuine VC6 UTILITY defines these after IOSFWD already supplied defaults.
    for name in (b"istreambuf_iterator", b"ostreambuf_iterator"):
        before = old + name
        after = b"template<class _E, class _Tr>\r\n\tclass " + name
        if before in raw:
            assert raw.count(before) == 1
            raw = raw.replace(before, after)
    if path.is_symlink():
        path.unlink()
    path.write_bytes(raw)


def build_lowercase_mirror(real: Path, mirror: Path) -> Path:
    """Recursive lowercase-symlink mirror of `real`, so <string.h> resolves.

    Every FOO.H under `real` gets a lowercase symlink `foo.h` (to the real,
    ABSOLUTE path) under `mirror`, preserving (lowercased) subdir structure.
    Rebuilt only when `real` changes (a `.src` marker guards it) so a
    toolchain bump does not leave dangling symlinks.
    """
    marker = mirror.parent / (mirror.name + ".src")
    if mirror.is_dir() and marker.is_file() and marker.read_text() == str(real):
        normalize_era_utility(mirror)
        return mirror
    if mirror.exists():
        shutil.rmtree(mirror)
    for root, _dirs, files in os.walk(real):
        rel = os.path.relpath(root, real)
        low = mirror if rel == "." else mirror / rel.lower()
        low.mkdir(parents=True, exist_ok=True)
        for fn in files:
            link = low / fn.lower()
            if not link.exists():
                link.symlink_to(os.path.join(root, fn))
    marker.parent.mkdir(parents=True, exist_ok=True)
    marker.write_text(str(real))
    normalize_era_utility(mirror)
    return mirror


def base_flags(msvc_inc: Path, dx_inc: Path | None,
               msvc_low: Path, dx_low: Path | None) -> list[str]:
    """The clang-cl flag set shared by every unit.

    /imsvc marks the toolchain headers as SYSTEM includes (diagnostics inside
    the era CRT/SDK headers are silenced). Lowercase mirrors FIRST so a
    lowercase `#include <string.h>` resolves; the real (uppercase) dirs follow
    for exact-case includes; DX before MSVC in both tiers so the selected SDK takes precedence.
    """
    return [
        f"--target={TARGET}",
        f"-fms-compatibility-version={MSC_COMPAT}",
        "-fms-extensions",
        # `&Temporary()` is MSVC C4238, a nonstandard extension the retail
        # sources use; clang errors on it by default.
        "-Wno-address-of-temporary",
        # Era MSVC accepts SDK HRESULT macros such as DIERR_INSUFFICIENTPRIVS as
        # signed switch labels even when their `long` literal is unsigned.
        "-Wno-c++11-narrowing",
        # Preserve MSVC's lazy template parsing semantics.
        "-fdelayed-template-parsing",
        *(["/imsvc", str(dx_low)] if dx_low else []),
        "/imsvc", str(msvc_low),
        *(["/imsvc", str(dx_inc)] if dx_inc else []),
        "/imsvc", str(msvc_inc),
        # our own headers - NOT /imsvc, so diagnostics in our code surface.
        "/I", str(INCLUDE),
        # vendored SDK headers (vendor/<sdk>/, one dir deep).
        *[f for d in (sorted(VENDOR.iterdir()) if VENDOR.is_dir() else []) if d.is_dir()
          for f in ("/I", str(d))],
        *DEFINES,
    ]


def unit_flags(flags: list[str]) -> list[str]:
    """Translate only parse-affecting switches; codegen stays with CL.EXE."""
    out = []
    i = 0
    while i < len(flags):
        flag = flags[i]
        if flag in ("/I", "/D", "/U", "/FI"):
            if i + 1 >= len(flags):
                raise ValueError(f"missing argument for {flag}")
            out += [flag, flags[i + 1]]
            i += 2
            continue
        if flag.startswith(("/D", "/U", "/I", "/FI", "/Zp")):
            out.append(flag)
        elif flag in ("/MT", "/MTd", "/MD", "/MDd"):
            out.append(flag)
        elif flag in ("/GX", "/GX-", "/GR", "/GR-"):
            out.append({"/GX": "/EHsc", "/GX-": "/EHs-c-"}.get(flag, flag))
        i += 1
    # Clang defaults RTTI on, whereas era CL requires /GR.
    if not any(f in ("/GX", "/GX-") or f.startswith("/EH") for f in flags):
        out.append("/EHs-c-")
    if not any(f in ("/GR", "/GR-") for f in flags):
        out.append("/GR-")
    return out


def generate(quiet: bool = False) -> bool:
    """(Re)write the compdb from config/units.toml. Returns True if changed."""
    from hobbit.manifest import load
    manifest = load()
    msvc_inc, dx_inc, provenance = resolve_include_dirs()
    msvc_low = build_lowercase_mirror(msvc_inc, MIRROR_DIR / "msvc")
    dx_low = build_lowercase_mirror(dx_inc, MIRROR_DIR / "dx") if dx_inc else None
    shared = base_flags(msvc_inc, dx_inc, msvc_low, dx_low)
    shared[1] = "-fms-compatibility-version=" + manifest.get("build", {}).get("msvc_compat", MSC_COMPAT)

    entries = [{
        "directory": str(REPO),
        "file": u["source"],
        # clang-cl driver form; clangd/clang parse it internally.
        "arguments": ["clang-cl", "/c", u["source"], *shared,
                      *unit_flags(manifest["flags"][u["flags"]])],
    } for u in manifest.get("unit", [])]

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    payload = json.dumps(entries, indent=2) + "\n"
    changed = not (OUT_FILE.exists()
                   and OUT_FILE.read_text(encoding="utf-8") == payload)
    if changed:
        OUT_FILE.write_text(payload, encoding="utf-8")
    if changed or not quiet:
        print(f"[compdb] {'wrote' if changed else 'unchanged'} "
              f"{OUT_FILE.relative_to(REPO)} ({len(entries)} units)")
    if not quiet:
        print(f"[compdb] include dirs ({provenance}):")
        print(f"    MSVC     : {msvc_inc}")
        print(f"    DirectX  : {dx_inc}")
        print(f"    lowercase mirrors -> {MIRROR_DIR}")
        print("[compdb] clang-cl flags per unit:")
        print("    clang-cl /c <src> " + " ".join(shared))
    return changed


def dead_include_dirs(db: dict) -> list[str]:
    """The `/imsvc` and `/I` directories the stored entries name that are GONE.

    A toolchain re-pin moves $MSVC_DIR/$DXSDK_DIR, and ninja cannot see that -
    no edge depends on the environment - so the compdb keeps naming the OLD
    /nix/store path. Once that path is garbage-collected every entry is
    unusable, and unit COVERAGE (which only asks whether a source has a row)
    still reports 300/300. Answering "full coverage" for a database that
    cannot resolve <string.h> is the lie this closes.
    """
    wanted: set[str] = set()
    for args in db.values():
        args = list(args)
        for i, arg in enumerate(args[:-1]):
            if arg in ("/imsvc", "-imsvc", "/I", "-I"):
                wanted.add(args[i + 1])
    return sorted(d for d in wanted if not os.path.isdir(d))


def check(quiet: bool = False) -> list[str]:
    """Coverage through the CONSUMER's parser: every manifest unit must have
    an entry in hobbit.tool.clang.compdb()'s dict - the exact join extraction
    performs, so a unit missing here is a unit that would silently fall back
    to bare MS flags. Returns the problems (empty = full coverage)."""
    from hobbit.manifest import units
    from hobbit.tool import clang
    db = clang.compdb()
    us = units()
    problems = []
    if not db:
        return [f"{OUT_FILE.relative_to(REPO)} is missing or unparsable - "
                f"EVERY unit would fall back to bare MS flags"]
    srcs = {}
    for u in us:
        srcs[os.path.realpath(str(REPO / u["source"]))] = u["unit"]
    missing = [unit for src, unit in srcs.items() if src not in db]
    problems += [f"unit '{u}' has NO compdb entry (bare-flag fallback)"
                 for u in sorted(missing)]
    stale = sorted(os.path.relpath(src, REPO) for src in db if src not in srcs)
    problems += [f"stale entry (not a manifest unit): {s}" for s in stale]
    dead = dead_include_dirs(db)
    problems += [f"include dir no longer exists: {d} - the toolchain moved; "
                 "re-run `python3 -m hobbit.graph.compdb`" for d in dead]
    if not quiet or problems:
        print(f"[compdb] coverage: {len(us) - len(missing)}/{len(us)} units "
              f"have an entry ({len(stale)} stale, {len(dead)} dead include "
              "dir(s))")
    return problems


def main() -> int:
    import argparse
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--check", action="store_true",
                    help="verify the existing file only; write nothing")
    ap.add_argument("--quiet", action="store_true",
                    help="print only changes and problems (the ninja edge)")
    a = ap.parse_args()
    if not a.check:
        generate(a.quiet)
    problems = check(a.quiet)
    for p in problems:
        print(f"[compdb] {p}", file=sys.stderr)
    return 1 if problems else 0


if __name__ == "__main__":
    raise SystemExit(main())
