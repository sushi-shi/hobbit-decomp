"""hobbit.retail_labels.fragments - extraction's per-TU cache, parse-only.

build/gen/claims/<unit>.tsv is extract's CACHE of the source macros (the
macros in src/ are the storage). Same Claim shape as the provider tables.
"""

from __future__ import annotations

from pathlib import Path

from hobbit.core.paths import BUILD
from hobbit.core.tsv import read as read_tsv
from hobbit.retail_labels import Claim

FRAGMENTS = BUILD / "gen/claims"

HEADER = ["rva", "size", "name", "kind", "channel", "type"]


def fragment_path(unit: str) -> Path:
    return FRAGMENTS / f"{unit}.tsv"


def unit_claims(unit: str) -> list[Claim]:
    path = fragment_path(unit)
    if not path.is_file():
        return []
    _b, _h, raw = read_tsv(path)
    out = []
    for r in raw:
        size = int(r["size"], 16) if r["size"].strip() else None
        meta = {"type": r["type"]} if r["type"].strip() else {}
        out.append(Claim(int(r["rva"], 16), r["name"], r["kind"],
                         r["channel"], size, unit, meta))
    return out


def stale_fragments() -> list[str]:
    """Ignored fragments cannot confer ownership after a unit is removed."""
    from hobbit.manifest import units
    configured = {u["unit"] for u in units()}
    return sorted(p.stem for p in FRAGMENTS.glob("*.tsv")
                  if p.stem not in configured)


def all_claims() -> list[Claim]:
    from hobbit.manifest import units
    out: list[Claim] = []
    for unit in units():
        out.extend(unit_claims(unit["unit"]))
    return out
