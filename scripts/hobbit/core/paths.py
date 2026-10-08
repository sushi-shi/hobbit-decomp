"""Repository paths, adapted from local Gruntz core/paths.py.

Donor revision: 7d4bd55b99e32f084834d991badf7609889481f4.
Keep only repository discovery; no compiler or SDK settings are assumed.
"""

from __future__ import annotations

import os
from pathlib import Path


def _find_repo() -> Path:
    for base in (Path.cwd(), Path(__file__).resolve().parent):
        for path in (base, *base.parents):
            if (path / "scripts/hobbit/manifest.py").is_file():
                return path
    raise RuntimeError("not inside a Hobbit tooling checkout")


REPO = _find_repo()
SRC = REPO / "src"
INCLUDE = REPO / "include"
VENDOR = REPO / "vendor"
CONFIG = REPO / "config"
RETAIL = CONFIG / "retail"
BUILD = REPO / "build"


def retail_exe() -> Path:
    return Path(os.environ.get("HOBBIT_EXE") or BUILD / "orig/Meridian.exe")


def msvc_dir() -> Path:
    value = os.environ.get("MSVC_DIR")
    if not value:
        raise RuntimeError("$MSVC_DIR unset - run inside `nix develop`")
    return Path(value)


def dxsdk_dir() -> Path:
    value = os.environ.get("DXSDK_DIR")
    if not value:
        raise RuntimeError("$DXSDK_DIR unset - configure a verified Hobbit SDK")
    return Path(value)
