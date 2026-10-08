"""Canonical Entropy sibling-lineage ledger reader, validator and queue renderer.

Git source identities use a revision and blob id. Released archives instead use
``archive-sha256:<digest>`` and ``sha256:<file-digest>``. For those rows,
``verify_blobs`` treats its source directory as the extraction root and checks
the named file payload only. An extraction cannot authenticate the archive's
digest; that provenance must be verified against the original archive separately.
"""

from __future__ import annotations

import hashlib
import json
import re
import subprocess
from pathlib import Path

from hobbit.core.paths import CONFIG
from hobbit.core.tsv import read, rint

LEDGER = CONFIG / "entropy_lineage.tsv"

HEADER = [
    "id",
    "wave",
    "source_commit",
    "source_blob",
    "source_path",
    "source_symbol",
    "hobbit_symbol",
    "rva",
    "module",
    "relation",
    "decision",
    "reason",
    "retail_evidence",
    "landed_commit",
]

RELATIONS = {
    "direct-family",
    "same-lineage-revision",
    "cross-game-copy",
    "structural-clone",
    "analogue-only",
}
DECISIONS = {"take", "take-adapted", "do-not-take", "pending"}
REASONS = {
    "retail-layout",
    "retail-abi",
    "retail-cfg",
    "retail-callset",
    "retail-referent",
    "retail-constant",
    "later-revision",
    "no-retail-owner",
    "analogue-only",
    "duplicate-source",
    "tooling-only",
}
TERMINAL = DECISIONS - {"pending"}
HEX_RE = re.compile(r"^[0-9a-f]{7,40}$")
SHA1_RE = re.compile(r"^[0-9a-f]{40}$")
ARCHIVE_RE = re.compile(r"^archive-sha256:[0-9a-f]{64}$")
TREE_RE = re.compile(r"^tree-sha256:[0-9a-f]{64}$")
FILE_SHA256_RE = re.compile(r"^sha256:[0-9a-f]{64}$")


def _source_identity_errors(commit: str, blob: str) -> list[str]:
    if commit.startswith("tree-sha256:"):
        errors = []
        if not TREE_RE.fullmatch(commit):
            errors.append("tree source_commit must be tree-sha256:<64 lowercase hex digits>")
        if not FILE_SHA256_RE.fullmatch(blob):
            errors.append("tree source_blob must be sha256:<64 lowercase hex digits>")
        return errors
    if commit.startswith("archive-sha256:") or blob.startswith("sha256:"):
        errors = []
        if not ARCHIVE_RE.fullmatch(commit):
            errors.append("archive source_commit must be archive-sha256:<64 lowercase hex digits>")
        if not FILE_SHA256_RE.fullmatch(blob):
            errors.append("archive source_blob must be sha256:<64 lowercase hex digits>")
        return errors
    errors = []
    if not HEX_RE.fullmatch(commit):
        errors.append("source_commit must be a 7-40 digit lowercase git id or tagged archive SHA256")
    if not SHA1_RE.fullmatch(blob):
        errors.append("Git source_blob must be a full lowercase git blob id")
    return errors


def load(path: Path = LEDGER) -> list[dict[str, str]]:
    _banner, header, rows = read(path)
    if header != HEADER:
        raise ValueError(
            f"{path}: header mismatch\n  expected: {' '.join(HEADER)}"
            f"\n  found:    {' '.join(header)}"
        )
    return rows


def validate_rows(rows: list[dict[str, str]], complete: bool = False) -> list[str]:
    errors: list[str] = []
    seen_ids: set[str] = set()
    seen_claims: set[tuple[str, str, str]] = set()
    for index, row in enumerate(rows, 2):
        prefix = f"row {index} ({row.get('id') or '?'})"
        row_id = row.get("id", "")
        if not re.fullmatch(r"[a-z0-9][a-z0-9-]*", row_id):
            errors.append(f"{prefix}: invalid id")
        elif row_id in seen_ids:
            errors.append(f"{prefix}: duplicate id")
        seen_ids.add(row_id)

        try:
            wave = int(row.get("wave", ""))
            if wave < 0 or wave > 99:
                raise ValueError
        except ValueError:
            errors.append(f"{prefix}: wave must be an integer from 0 through 99")

        commit = row.get("source_commit", "")
        blob = row.get("source_blob", "")
        errors.extend(f"{prefix}: {error}" for error in _source_identity_errors(commit, blob))
        if not row.get("source_path") or not row.get("source_symbol"):
            errors.append(f"{prefix}: source_path and source_symbol are required")

        claim = (commit, row.get("source_path", ""), row.get("source_symbol", ""))
        if claim in seen_claims:
            errors.append(f"{prefix}: duplicate source claim {claim[1]}:{claim[2]}")
        seen_claims.add(claim)

        relation = row.get("relation", "")
        decision = row.get("decision", "")
        reason = row.get("reason", "")
        evidence = row.get("retail_evidence", "")
        landed_field = row.get("landed_commit", "")
        landed = "" if landed_field == "-" else landed_field
        if relation not in RELATIONS:
            errors.append(f"{prefix}: unknown relation {relation!r}")
        if decision not in DECISIONS:
            errors.append(f"{prefix}: unknown decision {decision!r}")
            continue
        if row.get("rva"):
            try:
                rint(row["rva"])
            except ValueError:
                errors.append(f"{prefix}: invalid rva {row['rva']!r}")

        if decision == "pending":
            if reason or evidence or landed:
                errors.append(f"{prefix}: a pending row cannot claim a reason, evidence or commit")
            if complete:
                errors.append(f"{prefix}: pending decision remains")
        elif decision == "do-not-take":
            if reason not in REASONS:
                errors.append(f"{prefix}: do-not-take requires one controlled reason code")
            if not evidence:
                errors.append(f"{prefix}: do-not-take requires retail evidence")
            if landed:
                errors.append(f"{prefix}: do-not-take cannot carry a landed commit")
        else:
            if reason:
                errors.append(f"{prefix}: an adopted row cannot carry a rejection reason")
            if not landed or not HEX_RE.fullmatch(landed):
                errors.append(f"{prefix}: an adopted row requires its landed commit")
            if decision == "take-adapted" and not evidence:
                errors.append(f"{prefix}: take-adapted requires the retained retail evidence")
    return errors


def _git(source: Path, *args: str) -> str:
    proc = subprocess.run(
        ["git", "-C", str(source), *args],
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if proc.returncode:
        raise ValueError(proc.stderr.strip() or "git command failed")
    return proc.stdout.strip()


def verify_blobs(source: Path, rows: list[dict[str, str]]) -> list[str]:
    """Check Git blobs or extracted file payloads, not original archive digests."""
    errors: list[str] = []
    tree_rows = [row for row in rows if TREE_RE.fullmatch(row['source_commit'])]
    if tree_rows:
        from hobbit.lineage.discovery import tree_identity
        actual_tree = tree_identity(source)
        for identity in sorted({row['source_commit'] for row in tree_rows}):
            if identity != actual_tree:
                errors.append(f"source tree drift: ledger {identity}, source {actual_tree}")
    for row in rows:
        identity_errors = _source_identity_errors(row['source_commit'], row['source_blob'])
        if identity_errors:
            errors.extend(f"{row['id']}: {error}" for error in identity_errors)
            continue
        if ARCHIVE_RE.fullmatch(row['source_commit']) or TREE_RE.fullmatch(row['source_commit']):
            root = source.resolve()
            relative = Path(row['source_path'])
            path = (root / relative).resolve()
            if relative.is_absolute() or not path.is_relative_to(root):
                errors.append(
                    f"{row['id']}: archive source_path must stay relative to extraction root "
                    f"{root}: {row['source_path']}"
                )
                continue
            try:
                actual = "sha256:" + hashlib.sha256(path.read_bytes()).hexdigest()
            except OSError as exc:
                errors.append(
                    f"{row['id']}: cannot read extracted archive payload {path}: {exc}; "
                    "provide the extraction root containing source_path"
                )
                continue
            if actual != row['source_blob']:
                errors.append(
                    f"{row['id']}: extracted payload drift for {path}: "
                    f"ledger {row['source_blob']}, source {actual}; "
                    "check the release and extraction root (archive digest not verified here)"
                )
            continue
        spec = f"{row['source_commit']}:{row['source_path']}"
        try:
            actual = _git(source, "rev-parse", spec)
        except ValueError as exc:
            errors.append(f"{row['id']}: cannot resolve {spec}: {exc}")
            continue
        if actual != row["source_blob"]:
            errors.append(
                f"{row['id']}: blob drift for {spec}: ledger {row['source_blob']}, source {actual}"
            )
    return errors


def covered(candidate: dict[str, str], rows: list[dict[str, str]]) -> bool:
    """A file-level `*` claim covers discoveries within that exact source blob."""
    for row in rows:
        if row["source_commit"] != candidate["source_commit"]:
            continue
        if row["source_path"] != candidate["source_path"]:
            continue
        if row["source_blob"] != candidate["source_blob"]:
            continue
        if row["source_symbol"] in ("*", candidate["source_symbol"]):
            return True
    return False


def _baseline() -> dict[int, tuple[float, float, str]]:
    try:
        from hobbit.walls.inventory import baseline_rows

        return baseline_rows()
    except (OSError, ValueError):
        return {}


def queue_rows(rows: list[dict[str, str]], todo: bool = False,
               modules: set[str] | None = None) -> list[dict]:
    baseline = _baseline()
    out: list[dict] = []
    for row in rows:
        if todo and row["decision"] != "pending":
            continue
        row_modules = {part for part in row["module"].split(",") if part}
        if modules and not modules.intersection(row_modules):
            continue
        rva = rint(row["rva"]) if row["rva"] else None
        bank, hist, src_hash = baseline.get(rva, (None, None, ""))
        rendered = dict(row)
        rendered.update({"bank": bank, "hist": hist, "src_hash": src_hash})
        out.append(rendered)
    out.sort(
        key=lambda row: (
            int(row["wave"]),
            row["hist"] is None,
            row["hist"] if row["hist"] is not None else 101.0,
            rint(row["rva"]) if row["rva"] else 0xFFFFFFFF,
            row["id"],
        )
    )
    return out


def inventory_main(args) -> int:
    rows = queue_rows(load(), args.todo, set(args.module))
    if args.json:
        print(json.dumps(rows, indent=2))
        return 0
    pending = sum(row["decision"] == "pending" for row in rows)
    print(f"[lineage] {len(rows)} row(s), {pending} pending")
    print(f"{'wave':>4}  {'hist':>6}  {'decision':<13}  {'module':<10}  source -> Hobbit")
    for row in rows[: args.limit]:
        hist = f"{row['hist']:6.2f}" if row["hist"] is not None else "     -"
        target = row["hobbit_symbol"] or "-"
        print(
            f"{int(row['wave']):4d}  {hist}  {row['decision']:<13}  "
            f"{row['module'][:10]:<10}  {row['source_path']}:{row['source_symbol']} -> {target}"
        )
    if len(rows) > args.limit:
        print(f"  ... {len(rows) - args.limit} more (--limit)")
    return 0


def verify_main(args) -> int:
    try:
        rows = load()
    except (OSError, ValueError) as exc:
        print(f"[lineage] FATAL: {exc}")
        return 1
    errors = validate_rows(rows, args.complete)
    if args.source:
        source = Path(args.source).resolve()
        from hobbit.lineage.discovery import discover, tree_identity
        configured = CONFIG / "lineage_sources.json"
        sources = json.loads(configured.read_text()) if configured.exists() else {}
        expected = next((entry["source_identity"] for entry in sources.values()
                         if Path(entry["path"]).resolve() == source), None)
        if expected is None and not (source / ".git").exists():
            expected = tree_identity(source)
        selected = ([row for row in rows if row["source_commit"] == expected]
                    if expected else rows)
        if not selected:
            errors.append(f"no ledger rows identify source snapshot {source}")
        errors.extend(verify_blobs(source, selected))
        print(f"[lineage] source payload verification: {len(selected)} rows; "
              f"{len(rows) - len(selected)} rows belong to other source snapshots")
        candidates = discover(source, args.commit)
        for candidate in candidates:
            if not covered(candidate, rows):
                errors.append(
                    "unclassified candidate "
                    f"{candidate['source_path']}:{candidate['source_symbol']} "
                    f"({candidate['basis']})"
                )
    if errors:
        print(f"[lineage] FATAL: {len(errors)} finding(s)")
        for error in errors:
            print(f"  - {error}")
        return 1
    pending = sum(row["decision"] == "pending" for row in rows)
    print(f"[lineage] OK: {len(rows)} row(s), {pending} pending")
    return 0
