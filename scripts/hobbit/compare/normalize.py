"""Batch-normalize base + target COFF objs into disposable objdiff comparison copies.

    python3 -m hobbit.compare.normalize --base-dir B --target-dir T --out-dir O

A driver around `hobbit.compare.canonicalize.canonicalize_coff`: for every unit
it is given it rewrites the compiler-private data names (`$SG`/`$T`/`name$S<n>`),
resolves COFF weak externals to their default, and rewrites same-function
jump-table `DIR32` labels of both the recompiled base obj and its delinked
target obj into a content-addressed, side-by-side view under `<out-dir>/`.
The paired step states an explicit function size only when an identical
complete window proves the other side's extra terminal COMDAT NOPs are
alignment. Section payloads, including switch tables, remain intact.
Enrolled data extents also allow an exact one-past DIR32 address to adopt its
adjacent successor's name when both manifest and object prove the boundary;
the resolved section offset is unchanged and other addends remain distinct.
`objdiff.json` points at these copies; the real base and target objects are
never touched, so the transform is matching-NEUTRAL (see canonicalize.py and
the sibling homm2 docs/data-symbol-normalization).

Paired work is skipped when both copies and their sidecars are newer than
both inputs and the normalizer modules. Editing either input rebuilds that
unit's pair; `force=True` writes unconditionally. A `.symbols.tsv` sidecar is
emitted next to each copy, paired size proofs live in `function_sizes/`, and a
stamp lists the processed set. Boundary rewrites live in `data_boundaries/`;
changing the enrolled manifest invalidates its cached comparison copies.

The unit list is an ARGUMENT: callers pass the manifest census. This module
does not read config/units.toml, and it never predicts which units have a
target - `target_object` looks on disk.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path

from hobbit.compare import canonicalize as canon
from hobbit.compare import data_boundaries
from hobbit.compare import function_sizes
from hobbit.core import tsv
from hobbit.core.paths import BUILD
from hobbit.delink import eh_band

_MODULE_MTIME = max(
    Path(canon.__file__).stat().st_mtime,
    Path(canon.msvc_names.__file__).stat().st_mtime,
    Path(eh_band.__file__).stat().st_mtime,
    Path(function_sizes.__file__).stat().st_mtime,
    Path(data_boundaries.__file__).stat().st_mtime,
    Path(tsv.__file__).stat().st_mtime,
    Path(__file__).stat().st_mtime,
)
SYMBOL_SIZE = canon.SYMBOL_SIZE
#: The delinker spells its per-unit objects `<unit>.c.obj`; a plain `<unit>.obj`
#: is accepted too, so a target directory written either way still pairs.
TARGET_SUFFIXES = (".c.obj", ".obj")
DATA_MANIFEST = BUILD / "gen/delink_data_manifest.tsv"


def target_object(target_dir: Path, unit: str) -> Path | None:
    """The delinked object for `unit`, or None. Read off disk, never predicted."""
    for suffix in TARGET_SUFFIXES:
        path = target_dir / f"{unit}{suffix}"
        if path.exists():
            return path
    return None


def units_with_a_target(target_dir: Path) -> set[str]:
    """Unit stems the target directory actually holds an object for.

    THE OBJS ON DISK ARE THE ANSWER. Predicting this set from a name census is
    what once left data-only units pointing at the empty dummy.obj while their
    real target objs sat unopened beside it - a pairing objdiff scores 100.00%
    on every measure with zero totals, so the unit reports MATCHING while being
    entirely unscored.
    """
    if not target_dir.is_dir():
        return set()
    units: set[str] = set()
    for suffix in TARGET_SUFFIXES:
        units |= {p.name[:-len(suffix)] for p in target_dir.glob(f"*{suffix}")}
    return units


def _stale(src: Path, out: Path) -> bool:
    if not out.exists():
        return True
    out_mtime = out.stat().st_mtime
    return out_mtime < src.stat().st_mtime or out_mtime < _MODULE_MTIME


def _normalize_one(src: Path, out_obj: Path, out_sidecar: Path, *,
                   force: bool = False) -> str:
    """Normalize src -> out_obj (+ sidecar) when stale. Return a state token."""
    if not force and not _stale(src, out_obj) and not _stale(src, out_sidecar):
        return "skip"
    result = canon.canonicalize_coff(src.read_bytes())
    canon._atomic_write(out_obj, result.data)
    canon._atomic_write(out_sidecar, canon.sidecar_bytes(result.rows))
    return "wrote"


def _normalize_pair(base_src, target_src, base_obj, target_obj,
                    base_sidecar, target_sidecar, size_sidecar, boundary_sidecar,
                    manifest_rows, *, force=False):
    """Both inputs are dependencies of the paired size proof and both copies."""
    outputs = (base_obj, target_obj, base_sidecar, target_sidecar, size_sidecar,
               boundary_sidecar)
    if not force and all(not _stale(src, out) for src in (base_src, target_src)
                         for out in outputs):
        return ("skip", len(size_sidecar.read_bytes().splitlines()) - 1,
                len(boundary_sidecar.read_bytes().splitlines()) - 1)
    object_name = base_src.stem + ".c"
    base_payload, base_rewrites = data_boundaries.canonicalize_boundaries(
        base_src.read_bytes(), object_name, manifest_rows)
    target_payload, target_rewrites = data_boundaries.canonicalize_boundaries(
        target_src.read_bytes(), object_name, manifest_rows)
    base = canon.canonicalize_coff(base_payload)
    target = canon.canonicalize_coff(target_payload)
    base_data, target_data, proofs = function_sizes.paired_sizes(base.data, target.data)
    canon._atomic_write(base_obj, base_data)
    canon._atomic_write(target_obj, target_data)
    canon._atomic_write(base_sidecar, canon.sidecar_bytes(base.rows))
    canon._atomic_write(target_sidecar, canon.sidecar_bytes(target.rows))
    canon._atomic_write(size_sidecar, function_sizes.sidecar_bytes(proofs))
    canon._atomic_write(boundary_sidecar,
                        data_boundaries.sidecar_bytes(base_rewrites, target_rewrites))
    return "wrote", len(proofs), len(base_rewrites) + len(target_rewrites)


def _weak_and_strong_names(path: Path) -> tuple[set[str], set[str]]:
    """({weakly referenced}, {strongly defined}) symbol names of one COFF object.

    A raw symbol-table scan, not a full parse: the whole-link check below only
    needs the two name sets and runs over every processed object each build.
    """
    data = path.read_bytes()
    symptr, nsym = struct.unpack_from("<II", data, 8)
    strtab = symptr + nsym * SYMBOL_SIZE
    weak: set[str] = set()
    strong: set[str] = set()
    index = 0
    while index < nsym:
        base = symptr + index * SYMBOL_SIZE
        section, _typ, storage, aux = struct.unpack_from("<hHBB", data, base + 12)
        if struct.unpack_from("<I", data, base)[0]:
            name = data[base:base + 8].split(b"\0")[0].decode("latin1")
        else:
            offset = strtab + struct.unpack_from("<I", data, base + 4)[0]
            name = data[offset:data.index(b"\0", offset)].decode("latin1")
        if storage == canon.WEAK_EXTERNAL_STORAGE:
            weak.add(name)
        elif storage == canon.EXTERNAL_STORAGE and section > 0:
            strong.add(name)
        index += 1 + aux
    return weak, strong


def _assert_weak_externals_have_no_strong_definition(paths: list[Path]) -> int:
    """Fail if a weakly-referenced name is defined strongly anywhere.

    `canonicalize_coff` resolves each weak external to its auxiliary default,
    which is what the linker does ONLY while no object supplies a strong
    definition of that name. That is a whole-link fact, so it is re-proven here
    over the whole processed set rather than assumed per object. Measured today:
    508 weak `??_E<C>@@UAEPAXI@Z` references, 6 strong `??_E` definitions, and
    the two name sets are disjoint (the strong ones are non-virtual `QAEPAXI@Z`
    bodies and `W7AEPAXI@Z` thunks, a different mangling).
    """
    weak: set[str] = set()
    strong: set[str] = set()
    for path in paths:
        one_weak, one_strong = _weak_and_strong_names(path)
        weak |= one_weak
        strong |= one_strong
    clash = sorted(weak & strong)
    if clash:
        raise SystemExit(
            "[normalize] FATAL: %d weak external(s) also have a strong definition, "
            "so resolving them to their default is no longer what the linker does: %s"
            % (len(clash), ", ".join(clash[:8])))
    return len(weak)


def normalize(base_dir: Path, target_dir: Path, out_dir: Path,
              units: list[str], *, stamp: Path | None = None,
              force: bool = False, quiet: bool = False,
              data_manifest: Path | None = DATA_MANIFEST) -> dict:
    """Normalize both sides of `units` into `<out_dir>/{base,target}/`.

    Returns the counts the stamp records. `units` comes from the caller (the
    manifest census); which of them have a delinked target is read off disk.
    """
    base_dir, target_dir, out_dir = Path(base_dir), Path(target_dir), Path(out_dir)
    base_out = out_dir / "base"
    target_out = out_dir / "target"
    stamp = stamp if stamp is not None else out_dir / "normalize.stamp"

    manifest_rows = []
    if data_manifest is not None and Path(data_manifest).is_file():
        _banner, _header, manifest_rows = tsv.read(data_manifest)
        for row in manifest_rows:
            for field in ("rva", "size", "section_offset"):
                if row.get(field, "-") != "-":
                    row[field] = tsv.rint(row[field])
    # Hash the same parsed snapshot used below, rather than reading a table
    # twice while its producer may atomically replace it. Each pair owns its
    # digest: a partial run cannot certify another pair's old evidence.
    manifest_digest = hashlib.sha256(
        json.dumps(manifest_rows, sort_keys=True, separators=(",", ":"))
        .encode("utf-8")).hexdigest()
    wrote = skipped = base_n = target_n = size_n = boundary_n = 0
    processed: list[str] = []
    ordered = sorted(units)
    inputs = [p for unit in ordered
              for p in (base_dir / f"{unit}.obj", target_object(target_dir, unit))
              if p is not None and p.exists()]
    weak_n = _assert_weak_externals_have_no_strong_definition(inputs)

    for unit in ordered:
        base_src = base_dir / f"{unit}.obj"
        base_obj = base_out / f"{unit}.obj"
        base_sidecar = base_out / f"{unit}.symbols.tsv"
        target_src = target_object(target_dir, unit)
        target_sidecar = target_out / f"{unit}.symbols.tsv"
        size_sidecar = out_dir / "function_sizes" / f"{unit}.tsv"
        boundary_sidecar = out_dir / "data_boundaries" / f"{unit}.tsv"
        manifest_stamp = out_dir / "data_boundaries" / f"{unit}.sha256"
        manifest_changed = (not manifest_stamp.exists()
                            or manifest_stamp.read_text().strip() != manifest_digest)
        target_obj = target_out / target_src.name if target_src is not None else None
        if base_src.exists() and target_src is not None:
            state, proofs, boundaries = _normalize_pair(
                base_src, target_src, base_obj, target_obj, base_sidecar,
                target_sidecar, size_sidecar, boundary_sidecar, manifest_rows,
                force=force or manifest_changed)
            wrote += 2 * (state == "wrote")
            skipped += 2 * (state == "skip")
            size_n += proofs
            boundary_n += boundaries
            base_n += 1
            target_n += 1
            processed += [f"base/{unit}", f"target/{unit}"]
            canon._atomic_write(manifest_stamp, (manifest_digest + "\n").encode("ascii"))
        elif base_src.exists():
            state = _normalize_one(
                base_src, base_obj, base_sidecar,
                force=force or size_sidecar.exists() or boundary_sidecar.exists()
                or manifest_stamp.exists())
            wrote += state == "wrote"
            skipped += state == "skip"
            base_n += 1
            processed.append(f"base/{unit}")
        elif target_src is not None:
            state = _normalize_one(target_src, target_obj, target_sidecar,
                                   force=force or size_sidecar.exists()
                                   or boundary_sidecar.exists() or manifest_stamp.exists())
            wrote += state == "wrote"
            skipped += state == "skip"
            target_n += 1
            processed.append(f"target/{unit}")
        if not base_src.exists() or target_src is None:
            size_sidecar.unlink(missing_ok=True)
            boundary_sidecar.unlink(missing_ok=True)
            manifest_stamp.unlink(missing_ok=True)
        if not base_src.exists():
            base_obj.unlink(missing_ok=True)
            base_sidecar.unlink(missing_ok=True)
        # A unit that lost its delinked target (e.g. all names removed), or whose
        # target changed suffix, must not leave a stale normalized copy behind for
        # objdiff to pair against.
        stale_paths = [target_out / f"{unit}{suffix}" for suffix in TARGET_SUFFIXES]
        if target_src is None:
            stale_paths.append(target_sidecar)
        for stale in stale_paths:
            if stale != target_obj and stale.exists():
                stale.unlink()

    digest = hashlib.sha256("\n".join(processed).encode("utf-8")).hexdigest()
    counts = {"base_objects": base_n, "target_objects": target_n,
              "wrote": wrote, "skipped": skipped, "weak_externals": weak_n,
              "function_size_proofs": size_n,
              "data_boundary_rewrites": boundary_n,
              "set_sha256": digest}
    canon._atomic_write(
        stamp,
        ("# normalized objdiff comparison copies\n"
         f"base_objects\t{base_n}\n"
         f"target_objects\t{target_n}\n"
         f"wrote\t{wrote}\n"
         f"skipped\t{skipped}\n"
         f"function_size_proofs\t{size_n}\n"
         f"data_boundary_rewrites\t{boundary_n}\n"
         f"set_sha256\t{digest}\n").encode("utf-8"))
    if not quiet:
        print(f"[normalize] base={base_n} target={target_n} wrote={wrote} "
              f"skipped={skipped} weak-externals-resolved={weak_n} "
              f"function-size-proofs={size_n} data-boundary-rewrites={boundary_n}")
    return counts


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(
        prog="python3 -m hobbit.compare.normalize", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--base-dir", required=True, type=Path)
    ap.add_argument("--target-dir", required=True, type=Path)
    ap.add_argument("--out-dir", required=True, type=Path)
    ap.add_argument("--stamp", type=Path)
    ap.add_argument("--data-manifest", type=Path, default=DATA_MANIFEST,
                    help="enrolled data extents (missing file disables boundary normalization)")
    ap.add_argument("--force", action="store_true",
                    help="rewrite every copy, ignoring the stale check")
    ap.add_argument("--unit", action="append", default=[], dest="units",
                    help="repeatable; defaults to the config/units.toml census")
    args = ap.parse_args(argv)

    units = args.units
    if not units:
        from hobbit.manifest import units as manifest_units
        units = [u["unit"] for u in manifest_units()]
    normalize(args.base_dir, args.target_dir, args.out_dir, units,
              stamp=args.stamp, force=args.force, data_manifest=args.data_manifest)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
