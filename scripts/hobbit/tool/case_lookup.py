"""Read-only Clang case lookup; native compiler inputs remain unchanged.

Donors: Gruntz graph/compdb.py build_lowercase_mirror at
7d4bd55b99e32f084834d991badf7609889481f4; HoMM1 case-insensitive
include traversal at8d3ae6c96fc9b06d9c709fbdbfa181d78997b787.
A VFS view extends their spelling-only lookup to mixed-case and quoted
relative includes. Ambiguous Windows paths fail rather than select a file.
"""
from __future__ import annotations
import hashlib
import json
import os
import tempfile
from pathlib import Path
from hobbit.core.paths import BUILD, INCLUDE, SRC


class CaseLookupError(RuntimeError):
    pass


def include_roots(flags: list[str]) -> list[Path]:
    """Collect search roots without changing the driver's original order."""
    roots = []
    i = 0
    separated = {"/I", "-I", "/imsvc", "-imsvc", "-isystem", "-iquote", "/external:I"}
    joined = ("/external:I", "/imsvc", "-imsvc", "-isystem", "-iquote", "/I", "-I")
    while i < len(flags):
        flag = flags[i]
        if flag in separated:
            if i + 1 == len(flags):
                raise CaseLookupError(f"missing include directory after {flag}")
            roots.append(Path(flags[i + 1]).absolute())
            i += 2
            continue
        for prefix in joined:
            if flag.startswith(prefix) and len(flag) > len(prefix):
                roots.append(Path(flag[len(prefix):]).absolute())
                break
        i += 1
    return list(dict.fromkeys(roots))


def overlay_payload(roots: list[Path]) -> str:
    # Reject a collision when its header is actually requested. An unused SDK
    # collision must not invalidate unrelated source units. Generated entries
    # contain only #error diagnostics, never replacement declarations/bodies.
    groups = {}
    files = {}
    for root in dict.fromkeys(Path(r).absolute() for r in roots):
        if not root.is_dir():
            continue
        for parent, dirs, names in os.walk(root, followlinks=False):
            for name in sorted(dirs + names):
                path = Path(parent) / name
                spelling = str(path)
                groups.setdefault(spelling.casefold(), set()).add(spelling)
                if path.is_file():
                    files[spelling] = path
    collisions = {k: sorted(v) for k, v in groups.items() if len(v) > 1}
    errors = BUILD / "clangd/case-vfs/rejected"
    entries = {}
    for spelling, path in sorted(files.items()):
        conflicts = []
        # The explicit mirror namespace is an earlier search root whose
        # symlink pins one exact file. Do not change search precedence by
        # treating an ambiguity in a later physical root as one in this root.
        for owner in [path]:
            for component in [owner, *owner.parents]:
                conflicts += collisions.get(str(component).casefold(), [])
        external = spelling
        if conflicts:
            message = "ambiguous case-insensitive include: " + " / ".join(sorted(set(conflicts)))
            content = "#error " + json.dumps(message) + "\n"
            digest = hashlib.sha256(content.encode()).hexdigest()
            errors.mkdir(parents=True, exist_ok=True)
            diagnostic = errors / (digest + ".h")
            if not diagnostic.exists():
                fd, temporary = tempfile.mkstemp(dir=errors, suffix=".tmp")
                try:
                    with os.fdopen(fd, "w") as stream:
                        stream.write(content)
                    os.replace(temporary, diagnostic)
                finally:
                    Path(temporary).unlink(missing_ok=True)
            external = str(diagnostic.absolute())
        # Exactly one VFS entry for each folded name. Collision entries all
        # resolve to the diagnostic; no original definition is selected.
        entries[spelling.casefold()] = {"type": "file", "name": spelling,
                                        "external-contents": external}
    return json.dumps({"version": 0, "case-sensitive": False,
                       "use-external-names": False,
                       "roots": [entries[k] for k in sorted(entries)]},
                      sort_keys=True, separators=(",", ":")) + "\n"


def overlay_flags(flags: list[str], tu: str | None = None) -> list[str]:
    roots = [INCLUDE, SRC, *include_roots(flags)]
    if tu is not None:
        roots.append(Path(tu).absolute().parent)
    payload = overlay_payload(roots)
    digest = hashlib.sha256(payload.encode()).hexdigest()
    directory = BUILD / "clangd/case-vfs"
    directory.mkdir(parents=True, exist_ok=True)
    output = directory / (digest + ".json")
    if not output.exists():
        fd, temporary = tempfile.mkstemp(dir=directory, suffix=".tmp")
        try:
            with os.fdopen(fd, "w") as stream:
                stream.write(payload)
            os.replace(temporary, output)
        finally:
            Path(temporary).unlink(missing_ok=True)
    return ["-Xclang", "-ivfsoverlay", "-Xclang", str(output)]
