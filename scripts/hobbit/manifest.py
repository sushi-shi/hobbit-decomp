"""Read config/units.toml, the per-translation-unit build manifest.

Adapted from scripts/gruntz/manifest.py at Gruntz revision
7d4bd55b99e32f084834d991badf7609889481f4. The donor's data contract is unchanged.
"""

from __future__ import annotations

import tomllib
from pathlib import Path

from hobbit.core.paths import CONFIG


def load(path: Path | None = None) -> dict:
    with (path or CONFIG / "units.toml").open("rb") as stream:
        return tomllib.load(stream)


def units(path: Path | None = None) -> list[dict]:
    """[{unit, source, flags}] in manifest order."""
    return list(load(path).get("unit", []))


def flag_profiles(path: Path | None = None) -> dict[str, list[str]]:
    return dict(load(path).get("flags", {}))


def by_unit(path: Path | None = None) -> dict[str, dict]:
    return {u["unit"]: u for u in units(path)}
