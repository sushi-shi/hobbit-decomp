#!/usr/bin/env python3
"""Check reachable Git history (and optionally the index) before publication.

New Hobbit guard, complementary to HoMM1's snapshot publication mechanism in
scripts/homm1/clean/publish.py at 8d3ae6c96fc9b06d9c709fbdbfa181d78997b787.
This is a heuristic guard, not a guarantee against arbitrary encoded content or
obfuscated acquisition links. It does not inspect ignored/untracked local inputs
or unreachable objects/reflogs; those must never be included in a publication.
Only paths, object identifiers, and reason labels are printed, never blob text.
"""

from __future__ import annotations

import argparse
from collections import defaultdict
import os
from pathlib import Path, PurePosixPath
import re
import subprocess
import sys

MAX_BYTES = 100 * 1024 * 1024
LOCAL_ASSET_PATHS = frozenset({
    "src/xcore/auxiliary/bitmap/assets/defaultbmp_clut.inc",
    "src/xcore/auxiliary/bitmap/assets/defaultbmp_pixels.inc",
})
FORBIDDEN_SUFFIXES = frozenset("""
    .exe .dll .xbe .pdb .map .obj .o .a .lib .so .dylib .com .msi
    .zip .7z .rar .tar .gz .bz2 .xz .iso .bin .cue .img .cab
    .png .jpg .jpeg .gif .bmp .ico .tga .dds .xbmp .psd
    .wav .mp3 .ogg .aif .aiff .flac .bik .avi .mp4 .wmv
    .pak .wad .arc .rez .vpk .asm .s .disasm .dis .lst
""".split())
MAGIC = (
    b"MZ", b"\x7fELF", b"PK\x03\x04", b"PK\x05\x06", b"PK\x07\x08",
    b"XBEH", b"\x89PNG\r\n\x1a\n", b"\0\0\x01\0", b"\0\0\x02\0",
    b"Rar!", b"7z\xbc\xaf\x27\x1c", b"\x1f\x8b", b"!<arch>\n",
    b"GIF87a", b"GIF89a", b"\xff\xd8\xff", b"RIFF",
)
# Split known locators so this scanner and its fixtures are safe to publish.
# Keep open-source SDK, donor, and compiler/toolchain sources permitted.
LOCATORS = (
    rb"(?:www\.)?" + b"archive" + rb"\.org/(?:details|download)/",
    rb"(?:www\.)?" + b"myabandon" + rb"ware\.com/",
    rb"(?:www\.)?" + b"old-games" + rb"\.com/",
    rb"(?:www\.)?" + b"oldgames" + rb"download\.com/",
    rb"(?:www\.)?" + b"game" + rb"pressure\.com/download/",
    rb"(?:github\.com/)?" + b"hobbit-kingdom/" + b"hobbit-versions",
)
ACQUISITION = re.compile(b"|".join(LOCATORS), re.IGNORECASE)


def git(repo: Path, *args: str, input: bytes | None = None) -> bytes:
    return subprocess.run(["git", "-C", str(repo), *args], input=input,
                          env={**os.environ, "GIT_NO_REPLACE_OBJECTS": "1"},
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          check=True).stdout


def path_reasons(path: str) -> set[str]:
    parts = PurePosixPath(path.lower()).parts
    reasons = set()
    if path.lower() in LOCAL_ASSET_PATHS:
        reasons.add("original embedded asset include")
    if PurePosixPath(path.lower()).suffix in FORBIDDEN_SUFFIXES:
        reasons.add("binary, archive, or asset extension")
    if (PurePosixPath(path.lower()).suffix not in {".py", ".cpp", ".hpp", ".h"}
            and re.search(r"(?:^|\.)(?:asm|disasm|disassembly)(?:\.|$)", PurePosixPath(path.lower()).name)):
        reasons.add("assembly or disassembly dump")
    if path.lower().startswith("docs/") or "notes" in PurePosixPath(path.lower()).name or (
            path.lower().startswith("config/evidence/") and path.lower().endswith(".json")):
        reasons.add("notes or proof/review/replay dossier")
    if parts and parts[0] in {"build", "orig", "target"}:
        reasons.add("local/generated evidence directory")
    return reasons


def content_reasons(data: bytes, kind: str = "blob") -> set[str]:
    reasons = set()
    if kind == "blob" and (data.startswith(MAGIC) or b"\0" in data[:8192]):
        reasons.add("binary or asset content")
    if ACQUISITION.search(data):
        reasons.add("game acquisition locator")
    if kind == "blob" and data.startswith(b"version https://git-lfs.github.com/spec/"):
        reasons.add("Git LFS pointer requires separate payload review")
    return reasons


def audit(repo: Path, staged: bool = False) -> tuple[dict[str, set[str]], int]:
    if git(repo, "rev-parse", "--is-shallow-repository").strip() == b"true":
        raise ValueError("full history required; fetch with --unshallow first")
    findings: dict[str, set[str]] = defaultdict(set)
    paths: dict[str, set[str]] = defaultdict(set)
    object_ids = set(git(repo, "rev-list", "--objects", "--all", "--no-object-names").decode().split())
    if object_ids:
        # Enumerate every committed tree, including old path names and detached
        # HEAD. rev-list alone supplies only one name for a reused blob.
        roots = set(git(repo, "log", "--all", "--format=%T").decode().split())
        # Tags may point at trees directly, including through nested tags.
        for ref in git(repo, "for-each-ref", "--format=%(refname)").decode().splitlines():
            try:
                roots.add(git(repo, "rev-parse", "--verify", f"{ref}^{{tree}}").decode().strip())
            except subprocess.CalledProcessError:
                pass  # Blob tags still receive the content checks below.
        for root in roots:
            for entry in git(repo, "ls-tree", "-rz", root).split(b"\0"):
                if not entry:
                    continue
                meta, raw_path = entry.split(b"\t", 1)
                mode, kind, oid = meta.decode().split()
                path = raw_path.decode("utf-8", "backslashreplace")
                paths[oid].add(path)
                findings[path].update(path_reasons(path))
                if mode == "160000":
                    findings[path].add("submodule payload requires separate review")
    if staged:
        for entry in git(repo, "ls-files", "--stage", "-z").split(b"\0"):
            if not entry:
                continue
            meta, raw_path = entry.split(b"\t", 1)
            mode, oid, stage = meta.decode().split()
            path = raw_path.decode("utf-8", "backslashreplace")
            paths[oid].add(path)
            findings[path].update(path_reasons(path))
            if stage != "0":
                findings[path].add("unmerged index entry")
            if mode == "160000":
                findings[path].add("submodule payload requires separate review")
            else:
                object_ids.add(oid)

    # Batch metadata first: reject huge objects without loading them into RAM.
    metadata = git(repo, "cat-file", "--batch-check", input="".join(
        f"{oid}\n" for oid in sorted(object_ids)).encode())
    inspect = []
    for line in metadata.decode().splitlines():
        oid, kind, size = line.split()
        if int(size) >= MAX_BYTES:
            for path in paths[oid] or {f"object {oid}"}:
                findings[path].add("object is at least 100 MiB")
        else:
            inspect.append((oid, kind))
    # Communicate incrementally, avoiding both pipe deadlock and whole-history
    # buffering. Git returns a byte count, so embedded newlines are harmless.
    with subprocess.Popen(["git", "-C", str(repo), "cat-file", "--batch"],
                          env={**os.environ, "GIT_NO_REPLACE_OBJECTS": "1"},
                          stdin=subprocess.PIPE, stdout=subprocess.PIPE) as process:
        assert process.stdin is not None and process.stdout is not None
        for oid, kind in inspect:
            process.stdin.write(f"{oid}\n".encode())
            process.stdin.flush()
            header = process.stdout.readline().decode().split()
            data = process.stdout.read(int(header[2]))
            if process.stdout.read(1) != b"\n":
                raise ValueError("invalid Git batch response")
            if kind == "tree":
                # Also check entries in trees reachable only via a tag/ref.
                position = 0
                while position < len(data):
                    end = data.index(b"\0", position)
                    mode, name = data[position:end].split(b" ", 1)
                    name = name.decode("utf-8", "backslashreplace")
                    findings[f"tree {oid}: {name}"].update(path_reasons(name))
                    position = end + 1 + len(oid) // 2
            else:
                reasons = content_reasons(data, kind)
                for path in paths[oid] or {f"{kind} {oid}"}:
                    findings[path].update(reasons)
        process.stdin.close()
        if process.wait() != 0:
            raise ValueError("Git object inspection failed")
    return {path: reasons for path, reasons in findings.items() if reasons}, len(object_ids)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, default=Path("."))
    parser.add_argument("--staged", action="store_true", help="also audit every index entry")
    args = parser.parse_args()
    try:
        findings, count = audit(args.repo, args.staged)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"Public-tree audit could not finish: {error}", file=sys.stderr)
        return 2
    for path, reasons in sorted(findings.items()):
        print(f"{path!r}: {', '.join(sorted(reasons))}")
    print(f"Public-tree audit: {count} reachable/index objects; {len(findings)} findings.")
    return int(bool(findings))


if __name__ == "__main__":
    raise SystemExit(main())
