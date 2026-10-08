"""Entropy sibling-source discovery, adapted from Gruntz lineage discovery.

Git inputs use immutable revisions. The stripped local Entropy trees have no
Git metadata: hash their source-file snapshot and each candidate's raw bytes.
Neither a basename nor a structural similarity is a source adoption verdict.
"""

from __future__ import annotations

import hashlib
import json
import re
import subprocess
from pathlib import Path, PureWindowsPath

from hobbit.core.paths import INCLUDE, SRC, RETAIL
from hobbit.core.tsv import read
from hobbit.lineage.ledger import LEDGER, covered, load

SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".cxx", ".h", ".hpp"}
DIRECT_DIRS = ()
EXCLUDED_DIRS = ()
ENTITY_MARKERS = {}
COMMON_OWNERS = {
    "CArray",
    "CDialog",
    "CFile",
    "CObject",
    "CRect",
    "CString",
    "CTime",
    "CWnd",
    "ios",
    "istream",
    "ostream",
    "streambuf",
}
CPP_KEYWORDS = {
    "asm", "auto", "bool", "break", "case", "catch", "char", "class", "const",
    "continue", "default", "delete", "do", "double", "else", "enum", "explicit",
    "extern", "false", "float", "for", "friend", "goto", "if", "inline", "int",
    "long", "new", "operator", "private", "protected", "public", "register", "return",
    "short", "signed", "sizeof", "static", "struct", "switch", "template", "this",
    "throw", "true", "try", "typedef", "union", "unsigned", "virtual", "void",
    "volatile", "while",
}
TOKEN_RE = re.compile(
    r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[A-Za-z_][A-Za-z0-9_]*|'
    r"0[xX][0-9A-Fa-f]+|\d+(?:\.\d+)?|==|!=|<=|>=|->|::|&&|\|\||<<|>>|\S"
)
OWNER_RE = re.compile(r"\b([A-Za-z_][A-Za-z0-9_]*)::")
STRING_RE = re.compile(r'"((?:\\.|[^"\\]){16,})"')
COMMENT_RE = re.compile(r"/\*.*?\*/|//[^\n]*", re.S)


def _git(source: Path, *args: str) -> str:
    proc = subprocess.run(
        ["git", "-C", str(source), *args],
        check=False,
        text=True,
        encoding="latin-1",
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if proc.returncode:
        raise SystemExit(proc.stderr.strip() or "git command failed")
    return proc.stdout


def _working_files(source: Path) -> list[str]:
    return sorted(path.relative_to(source).as_posix() for path in source.rglob("*")
                  if path.is_file() and path.suffix.lower() in SOURCE_SUFFIXES
                  and ".git" not in path.relative_to(source).parts)


def tree_identity(source: Path, files: list[str] | None = None) -> str:
    """SHA256 of sorted relative source paths and each payload SHA256.

    Covers SOURCE_SUFFIXES only, deliberately excluding stripped art/build
    products. This identifies local evidence, not an upstream Git revision.
    """
    digest = hashlib.sha256()
    for name in _working_files(source) if files is None else files:
        digest.update(name.encode("utf-8") + b"\0")
        digest.update(hashlib.sha256((source / name).read_bytes()).digest())
    return "tree-sha256:" + digest.hexdigest()


def _tree_files(source: Path, commit: str) -> list[str]:
    if commit.startswith("tree-sha256:"):
        return _working_files(source)
    out = _git(source, "ls-tree", "-r", "--name-only", commit)
    return sorted(
        path for path in out.splitlines() if Path(path).suffix.lower() in SOURCE_SUFFIXES
    )


def _blob(source: Path, commit: str, path: str) -> str:
    if commit.startswith("tree-sha256:"):
        return "sha256:" + hashlib.sha256((source / path).read_bytes()).hexdigest()
    return _git(source, "rev-parse", f"{commit}:{path}").strip()


def _text_at(source: Path, commit: str, path: str) -> str:
    if commit.startswith("tree-sha256:"):
        return (source / path).read_bytes().decode("latin-1")
    return _git(source, "show", f"{commit}:{path}")


def _local_files() -> list[Path]:
    return sorted(
        path
        for root in (SRC, INCLUDE)
        for path in root.rglob("*")
        if path.is_file() and path.suffix.lower() in SOURCE_SUFFIXES
    )


def _tokens(text: str) -> list[str]:
    text = COMMENT_RE.sub(" ", text)
    out: list[str] = []
    for token in TOKEN_RE.findall(text):
        if token[0].isalpha() or token[0] == "_":
            # Preserve API/type vocabulary while erasing ordinary local names.
            # Replacing every identifier made unrelated class declarations look
            # like clones; era sibling copies usually retain their owner,
            # callee, typedef and member spelling even when locals drift.
            semantic = "_" in token or any(ch.isupper() for ch in token)
            out.append(token if token in CPP_KEYWORDS or semantic else "ID")
        elif token[0].isdigit():
            out.append("NUM")
        elif token[0] in "\"'":
            out.append("STR")
        else:
            out.append(token)
    return out


def _shingles(tokens: list[str], width: int, stride: int = 16) -> set[bytes]:
    return {
        hashlib.sha1(" ".join(tokens[start : start + width]).encode()).digest()
        for start in range(0, max(0, len(tokens) - width + 1), stride)
    }


def structural_clone(local_text: str, source_text: str) -> bool:
    local = _tokens(local_text)
    source = _tokens(source_text)
    long_hits = _shingles(local, 96).intersection(_shingles(source, 96))
    if long_hits:
        return True
    return len(_shingles(local, 48).intersection(_shingles(source, 48))) >= 2


def discover(source: Path, commit: str | None = None) -> list[dict[str, str]]:
    source = source.resolve()
    if not source.is_dir():
        raise ValueError(f"source tree missing: {source}")
    if commit is None:
        commit = (_git(source, "rev-parse", "HEAD").strip()
                  if (source / ".git").exists() else tree_identity(source))
    elif not commit.startswith("tree-sha256:"):
        commit = _git(source, "rev-parse", "--verify", commit + "^{commit}").strip()
    elif tree_identity(source) != commit:
        raise ValueError(f"source tree identity does not match {commit}")
    files = _tree_files(source, commit)
    local_paths = _local_files()
    local_text = {path: path.read_text(errors="ignore") for path in local_paths}
    local_basenames = {path.name.lower() for path in local_paths}
    # Original PC paths and Xbox object names are useful even before their
    # source has been reconstructed. Keep these weak signals explicit.
    pc_basenames = set()
    xbox_stems = set()
    pc_paths = RETAIL / "pc/source_paths.tsv"
    xbox_objects = RETAIL / "xbox/objects.tsv"
    if pc_paths.exists():
        pc_basenames = {PureWindowsPath(row["path"]).name.lower()
                        for row in read(pc_paths)[2]}
    if xbox_objects.exists():
        xbox_stems = {PureWindowsPath(row["object"]).stem.lower()
                      for row in read(xbox_objects)[2] if row["object"].lower().endswith(".obj")}
    local_owners = {
        owner
        for text in local_text.values()
        for owner in OWNER_RE.findall(text)
        if owner not in COMMON_OWNERS
    }
    local_strings = {
        value
        for text in local_text.values()
        for value in STRING_RE.findall(text)
        if any(ch.isalpha() for ch in value)
        if not value.lower().startswith(("c:\\\\proj\\\\", "software\\\\"))
    }

    # Structural comparison is expensive. Index only local token shingles once.
    local_48: set[bytes] = set()
    local_96: set[bytes] = set()
    for text in local_text.values():
        tokens = _tokens(text)
        local_48.update(_shingles(tokens, 48))
        local_96.update(_shingles(tokens, 96))

    found: dict[tuple[str, str], dict[str, str]] = {}
    for path in files:
        if path.startswith(EXCLUDED_DIRS):
            continue
        direct = any(path == root or path.startswith(root + "/") for root in DIRECT_DIRS)
        same_basename = Path(path).name.lower() in local_basenames
        pc_basename = Path(path).name.lower() in pc_basenames
        xbox_basename = Path(path).stem.lower() in xbox_stems
        if not direct and not same_basename and not pc_basename and not xbox_basename:
            # Avoid reading all later game files unless a cheap path/name signal exists.
            cheap_name = any(owner.lower() in path.lower() for owner in local_owners)
            if not cheap_name:
                continue
        text = _text_at(source, commit, path)
        entities = [
            symbol
            for symbol, landmarks in ENTITY_MARKERS.items()
            if symbol in text and all(landmark in text for landmark in landmarks)
        ]
        owners = sorted(set(OWNER_RE.findall(text)).intersection(local_owners))
        strings = sorted(
            value for value in set(STRING_RE.findall(text)).intersection(local_strings)
            if any(ch.isalpha() for ch in value)
        )
        tokens = _tokens(text)
        clone = bool(local_96.intersection(_shingles(tokens, 96))) or (
            len(local_48.intersection(_shingles(tokens, 48))) >= 2
        )
        bases: list[str] = []
        if direct:
            bases.append("direct-family closure")
        if same_basename:
            bases.append("exact basename")
        if pc_basename:
            bases.append("PC original source basename")
        if xbox_basename:
            bases.append("Xbox object basename")
        if owners:
            bases.append("shared owner " + ",".join(owners[:4]))
        if strings:
            bases.append("shared literal " + repr(strings[0][:48]))
        if clone:
            bases.append("normalized structural clone")
        if not bases and not entities:
            continue
        relation = "direct-family" if direct else (
            "structural-clone" if clone else "analogue-only"
        )
        blob = _blob(source, commit, path)
        if bases:
            key = (path, "*")
            found[key] = {
                "source_commit": commit,
                "source_blob": blob,
                "source_path": path,
                "source_symbol": "*",
                "relation": relation,
                "basis": "; ".join(bases),
            }
        for symbol in entities:
            key = (path, symbol)
            found[key] = {
                "source_commit": commit,
                "source_blob": blob,
                "source_path": path,
                "source_symbol": symbol,
                "relation": "cross-game-copy",
                "basis": "named repeated implementation; "
                         + ",".join(ENTITY_MARKERS[symbol]),
            }
    return sorted(found.values(), key=lambda row: (row["source_path"], row["source_symbol"]))


def main(args) -> int:
    source = Path(args.source)
    candidates = discover(source, args.commit)
    rows = load() if LEDGER.is_file() else []
    if not args.all:
        candidates = [candidate for candidate in candidates if not covered(candidate, rows)]
    if args.json:
        print(json.dumps(candidates, indent=2))
        return 0
    label = "candidate" if args.all else "unclassified candidate"
    print(f"[lineage] {len(candidates)} {label}(s)")
    for candidate in candidates:
        print(
            f"{candidate['source_blob']}  {candidate['source_path']}:"
            f"{candidate['source_symbol']}  [{candidate['relation']}] {candidate['basis']}"
        )
    return 0
