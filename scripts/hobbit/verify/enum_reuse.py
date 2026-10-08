"""hobbit.verify.enum_reuse - evaluated enum and constant-reuse census.

Equal integer values are review leads, never proof that two domains are the
same.  This audit combines two views:

* a lexical inventory of enum blocks, object-like macro definitions and
  scalar const declarations under ``src/`` and ``include/``, including
  inactive/unreferenced source; and
* a libclang pass over every project translation unit, which evaluates aliases,
  shifts, negative values, character constants, and implicit increments with
  the target ABI. Compiler-evaluated named values share the existing ledger.

The two views must cover one another.  A declaration missing from the evaluated
view is a fatal coverage hole rather than a silently incomplete report.
Named non-integral/runtime exclusions and unresolved definitions are reported
separately; an incomplete scan writes explicitly partial numeric reports.

    hobbit verify enum-reuse                  # write the three derived TSV reports
    hobbit verify enum-reuse --duplicates     # print values declared twice+
    hobbit verify enum-reuse --value 10       # inspect one value
    hobbit verify enum-reuse --json           # machine-readable full census
    hobbit verify enum-reuse --init-ledger    # snapshot the review worklist
    hobbit verify enum-reuse --extend-ledger  # append new pending domains only
"""

from __future__ import annotations

import argparse
import csv
import ctypes
import json
import multiprocessing
import re
import sys
from collections import Counter, defaultdict
from concurrent.futures import ProcessPoolExecutor
from dataclasses import asdict, dataclass, replace
from itertools import combinations
from pathlib import Path

from hobbit.core.paths import BUILD, REPO
from hobbit.verify.constants import (
    _CHAR, _STRING, _SOURCE_EXTENSIONS, _blank_literal, _flags,
    _require_cl_mode, _source_path, source_stamp,
)
from hobbit.verify.constant_context import semantic_context
from hobbit.verify.srcscan import blank_comments


CDB = BUILD / "clangd/compile_commands.json"
REPORT = BUILD / "gen/enum_reuse.tsv"
COLLISION_REPORT = BUILD / "gen/enum_value_collisions.tsv"
PAIR_REPORT = BUILD / "gen/enum_domain_pairs.tsv"
NAMED_REPORT = BUILD / "gen/named_constant_coverage.tsv"
BARE_CONSTANTS = BUILD / "gen/bare_constants.tsv"
LEDGER = REPO / "config/reviews/enum-reuse.tsv"

LEDGER_FIELDS = (
    "source_enum", "members", "decision", "current_enums", "member_reuse",
    "reason",
)
LEDGER_DECISIONS = frozenset(("pending", "retain", "canonical", "reuse", "replace"))

_MACRO_BLOCK = re.compile(
    r"\bGZ_ENUM_(BEGIN|BEGIN_SPLIT|FLAGS_BEGIN|CONST_BEGIN)"
    r"\(\s*(\w+)\s*(?:,\s*(\w+)\s*)?\)(?P<body>.*?)"
    r"\bGZ_ENUM_(?:END|END_SPLIT|FLAGS_END|CONST_END)\(",
    re.S,
)
_RAW_ENUM = re.compile(
    r"\b(?:typedef\s+)?enum\s+(?:(?:class|struct)\s+)?"
    r"(?P<name>[A-Za-z_]\w*)?\s*\{"
)
_MEMBER_NAME = re.compile(r"[A-Za-z_]\w*")


@dataclass(frozen=True)
class Member:
    name: str
    line: int
    column: int
    offset: int
    expression: str


@dataclass(frozen=True)
class Block:
    source_enum: str
    domain: str
    kind: str
    storage: str
    file: str
    line: int
    offset: int
    end_offset: int
    members: tuple[Member, ...]


@dataclass(frozen=True)
class RawConstant:
    value: int | None
    name: str
    file: str
    line: int
    column: int
    offset: int
    parent_name: str
    parent_file: str
    parent_offset: int
    context: str
    kind: str = "enum"
    expression: str = ""
    status: str = "evaluated"
    reason: str = ""
    use_contexts: tuple[str, ...] = ()


@dataclass(frozen=True)
class Constant:
    value: int
    name: str
    file: str
    line: int
    column: int
    offset: int
    source_enum: str
    domain: str
    kind: str
    storage: str
    expression: str
    contexts: tuple[str, ...]
    use_contexts: tuple[str, ...] = ()


_DEFINE = re.compile(r"^[ \t]*#[ \t]*define[ \t]+(?P<name>[A-Za-z_]\w*)(?P<body>(?:\\\n|[^\n])*)", re.M)
_TYPE_WORDS = r"(?:unsigned\s+|signed\s+)?[A-Za-z_]\w*(?:::\w+)*(?:\s+(?:long|int|char))?"
_CONST = re.compile(
    rf"\b(?:const\s+(?P<prefix>{_TYPE_WORDS})|(?P<postfix>{_TYPE_WORDS})\s+const)"
    r"\s+(?P<body>[A-Za-z_]\w*\s*=[^;]+);"
)


def _named_blocks(path: Path, repo: Path) -> list[Block]:
    """Lexical coverage witnesses, not an expression evaluator.

    The AST adds declarations with other legal spellings (including comma
    declarators). These witnesses also expose declarations in unused headers.
    """
    text = blank_comments(path.read_text(errors="replace"))
    rel = str(path.resolve().relative_to(repo.resolve()))
    result = []
    seen = Counter()
    for match in _DEFINE.finditer(text):
        kind = "define"
        body = match.group("body")
        if body.startswith("(") or not body.strip():
            continue  # function-like macros and empty include guards
        expression = body.replace("\\\n", " ").strip()
        offset = match.start("name")
        line, column = _line_column(text, offset)
        name = match.group("name")
        member = Member(name, line, column, offset, expression)
        domain = f"<{kind}:{name}>"
        result.append(Block(
            _source_enum(rel, domain, line, (member,), seen), domain, kind,
            "", rel, line, match.start(), match.end(), (member,),
        ))
    for match in _CONST.finditer(text):
        for member in _members(text, match.start("body"), match.end("body")):
            tail = text[member.offset + len(member.name):match.end("body")].lstrip()
            if not tail.startswith("="):
                continue  # another parameter's type is not a comma declarator
            domain = f"<const-integral:{member.name}>"
            result.append(Block(
                _source_enum(rel, domain, member.line, (member,), seen), domain,
                "const-integral", match.group("prefix") or match.group("postfix"),
                rel, member.line, match.start(), match.end(), (member,),
            ))
    return result


def _integral(cidx, ty) -> bool:
    return ty.get_canonical().kind in {
        cidx.TypeKind.BOOL, cidx.TypeKind.CHAR_U, cidx.TypeKind.UCHAR,
        cidx.TypeKind.CHAR_S, cidx.TypeKind.SCHAR, cidx.TypeKind.WCHAR,
        cidx.TypeKind.USHORT, cidx.TypeKind.UINT, cidx.TypeKind.ULONG,
        cidx.TypeKind.ULONGLONG, cidx.TypeKind.SHORT, cidx.TypeKind.INT,
        cidx.TypeKind.LONG, cidx.TypeKind.LONGLONG, cidx.TypeKind.ENUM,
    }


def _evaluate_integer(cidx, node) -> int | None:
    """Use Clang's target-ABI evaluator; never interpret C++ in Python."""
    lib = cidx.conf.lib
    lib.clang_Cursor_Evaluate.argtypes = [cidx.Cursor]
    lib.clang_Cursor_Evaluate.restype = ctypes.c_void_p
    for name, result in (("clang_EvalResult_getKind", ctypes.c_int),
                         ("clang_EvalResult_isUnsignedInt", ctypes.c_uint),
                         ("clang_EvalResult_getAsLongLong", ctypes.c_longlong),
                         ("clang_EvalResult_getAsUnsigned", ctypes.c_ulonglong)):
        function = getattr(lib, name)
        function.argtypes = [ctypes.c_void_p]
        function.restype = result
    lib.clang_EvalResult_dispose.argtypes = [ctypes.c_void_p]
    lib.clang_EvalResult_dispose.restype = None
    value = lib.clang_Cursor_Evaluate(node)
    if not value:
        return None
    try:
        if lib.clang_EvalResult_getKind(value) != 1:  # CXEval_Int
            return None
        if lib.clang_EvalResult_isUnsignedInt(value):
            return lib.clang_EvalResult_getAsUnsigned(value)
        return lib.clang_EvalResult_getAsLongLong(value)
    finally:
        lib.clang_EvalResult_dispose(value)


def _runtime_initializer(cidx, node) -> bool:
    """Recognize runtime dependencies; evaluator failure alone proves none."""
    for child in node.walk_preorder():
        if child.kind == cidx.CursorKind.CALL_EXPR:
            return True  # this target uses C++98, without constexpr functions
        if child.kind == cidx.CursorKind.MEMBER_REF_EXPR and child.referenced and child.referenced.kind == cidx.CursorKind.FIELD_DECL:
            return True
        if child.kind == cidx.CursorKind.DECL_REF_EXPR and child.referenced:
            target = child.referenced
            if target.kind == cidx.CursorKind.PARM_DECL:
                return True
            if target.kind == cidx.CursorKind.VAR_DECL and (
                    not target.type.is_const_qualified() or target.type.is_volatile_qualified()):
                return True
    return False


def _project_files(repo: Path):
    for root_name in ("include", "src"):
        root = repo / root_name
        if not root.is_dir():
            continue
        for path in sorted(root.rglob("*")):
            if path.is_file() and path.suffix in (".h", ".hpp", ".inl", ".c", ".cc", ".cpp", ".cxx", ".rc"):
                yield path


def _line_column(text: str, offset: int) -> tuple[int, int]:
    line = text.count("\n", 0, offset) + 1
    column = offset - text.rfind("\n", 0, offset)
    return line, column


def _matching_brace(text: str, opening: int) -> int:
    depth = 0
    for offset in range(opening, len(text)):
        if text[offset] == "{":
            depth += 1
        elif text[offset] == "}":
            depth -= 1
            if depth == 0:
                return offset
    raise ValueError(f"unterminated enum opening at byte {opening}")


def _members(text: str, body_start: int, body_end: int) -> tuple[Member, ...]:
    """Split an enum body on top-level commas and retain source offsets."""
    boundaries = []
    start = body_start
    depth = 0
    quote = ""
    escaped = False
    for offset in range(body_start, body_end):
        char = text[offset]
        if quote:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == quote:
                quote = ""
            continue
        if char in "\"'":
            quote = char
        elif char in "([{":
            depth += 1
        elif char in ")]}" and depth:
            depth -= 1
        elif char == "," and depth == 0:
            boundaries.append((start, offset))
            start = offset + 1
    boundaries.append((start, body_end))

    result = []
    for start, end in boundaries:
        segment = text[start:end]
        match = _MEMBER_NAME.search(segment)
        if match is None:
            continue
        offset = start + match.start()
        line, column = _line_column(text, offset)
        source = text[offset:end].strip()
        expression = source.split("=", 1)[1].strip() if "=" in source else ""
        result.append(Member(match.group(), line, column, offset, expression))
    return tuple(result)


def _source_enum(rel: str, domain: str, line: int, members, seen: Counter) -> str:
    anonymous = members[0].name if members else f"line-{line}"
    base = f"{rel}:{domain}" if domain else f"{rel}:<anonymous:{anonymous}>"
    seen[base] += 1
    return base if seen[base] == 1 else f"{base}#{seen[base]}"


def scan_blocks(*, repo: Path = REPO, paths=None) -> list[Block]:
    """Inventory source enum blocks without preprocessing."""
    blocks = []
    seen: Counter = Counter()
    for path in list(paths) if paths is not None else _project_files(repo):
        blocks.extend(_named_blocks(path, repo))
        if path.resolve() == (repo / "include/Enums.h").resolve():
            continue
        original = path.read_text(errors="replace")
        text = blank_comments(original)
        rel = str(path.resolve().relative_to(repo.resolve()))
        occupied = []
        for match in _MACRO_BLOCK.finditer(text):
            kind, domain, storage = match.group(1), match.group(2), match.group(3) or ""
            body_start, body_end = match.start("body"), match.end("body")
            line, _ = _line_column(text, match.start())
            members = _members(text, body_start, body_end)
            blocks.append(Block(
                _source_enum(rel, domain, line, members, seen), domain,
                kind.lower(), storage,
                rel, line, match.start(), match.end(), members,
            ))
            occupied.append((match.start(), match.end()))

        for match in _RAW_ENUM.finditer(text):
            if any(start <= match.start() < end for start, end in occupied):
                continue
            line_start = text.rfind("\n", 0, match.start()) + 1
            if text[line_start:match.start()].lstrip().startswith("#"):
                continue
            opening = text.find("{", match.start(), match.end())
            closing = _matching_brace(text, opening)
            domain = match.group("name") or ""
            line, _ = _line_column(text, match.start())
            members = _members(text, opening + 1, closing)
            blocks.append(Block(
                _source_enum(rel, domain, line, members, seen), domain, "raw", "", rel,
                line, match.start(), closing + 1, members,
            ))
    return sorted(blocks, key=lambda block: (block.file, block.offset))


def _scan_entry(payload):
    entry, repo_text = payload
    repo = Path(repo_text)
    import clang.cindex as cidx

    path = _source_path(entry, repo)
    args = _flags(entry)
    try:
        _require_cl_mode(args)
    except RuntimeError as exc:
        return [], f"{path}: {exc}"
    try:
        tu = cidx.Index.create().parse(str(path), args=args,
            options=cidx.TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD)
    except cidx.TranslationUnitLoadError as exc:
        return [], f"{path}: libclang could not load TU: {exc}"
    errors = [diag for diag in tu.diagnostics if diag.severity >= cidx.Diagnostic.Error]
    if errors:
        return [], f"{path}: parse error: {errors[0]}"

    context = str(path.relative_to(repo))
    constants = []
    relative_paths = {}

    def project_path(name):
        if name not in relative_paths:
            try:
                rel = str(Path(name).resolve().relative_to(repo))
                relative_paths[name] = rel if rel.startswith(("src/", "include/")) else None
            except ValueError:
                relative_paths[name] = None
        return relative_paths[name]

    nodes = list(tu.cursor.walk_preorder())
    included = {path.resolve()}
    for inclusion in tu.get_includes():
        included.add(Path(inclusion.include.name).resolve())
    for node in nodes:
        if node.kind not in (cidx.CursorKind.ENUM_CONSTANT_DECL, cidx.CursorKind.VAR_DECL, cidx.CursorKind.PARM_DECL) or node.location.file is None:
            continue
        rel = project_path(node.location.file.name)
        if rel is None:
            continue
        kind, expression, status, reason = "enum", "", "evaluated", ""
        if node.kind in (cidx.CursorKind.VAR_DECL, cidx.CursorKind.PARM_DECL):
            if not node.type.is_const_qualified():
                continue
            children = list(node.get_children())
            if not any(child.kind.is_expression() for child in children):
                continue  # extern declarations have no defining value
            kind = "const-integral"
            tokens = [token.spelling for token in node.get_tokens()]
            expression = " ".join(tokens[tokens.index("=") + 1:]) if "=" in tokens else ""
            if node.kind == cidx.CursorKind.PARM_DECL:
                value, status, reason = None, "excluded", "default parameter is not a named constant definition"
            elif not _integral(cidx, node.type):
                value, status, reason = None, "excluded", "const declaration has non-integral type"
            else:
                value = _evaluate_integer(cidx, node)
                if value is None:
                    if _runtime_initializer(cidx, node):
                        status, reason = "excluded", "integral const initializer depends on runtime values"
                    else:
                        status, reason = "unresolved", "integral const initializer could not be evaluated; runtime status unproven"
        else:
            value = node.enum_value
        parent = node.semantic_parent
        parent_file = ""
        parent_offset = -1
        parent_name = ""
        if parent is not None:
            parent_name = parent.spelling
            if parent.location.file is not None:
                parent_file = project_path(parent.location.file.name) or ""
                parent_offset = parent.location.offset
        constants.append(RawConstant(
            value, node.spelling, rel, node.location.line,
            node.location.column, node.location.offset, parent_name,
            parent_file, parent_offset, context, kind, expression, status, reason,
        ))
    constants.extend(_evaluate_macros(cidx, path, args, included, repo, context))
    use_contexts, macro_roles = _named_uses(cidx, tu, repo,
        {(row.file, row.offset) for row in constants if row.status != "excluded" or row.kind == "define"})
    updated = []
    for row in constants:
        key = row.file, row.offset
        row = replace(row, use_contexts=tuple(sorted(use_contexts.get(key, ()))))
        if row.kind == "define" and row.status != "evaluated" and key in macro_roles:
            row = replace(row, status="excluded", reason=macro_roles[key])
        updated.append(row)
    constants = updated
    return constants, None


def _named_uses(cidx, tu, repo, definitions):
    uses = defaultdict(set)
    expansions = defaultdict(set)
    macro_roles = {}
    relative_paths = {}

    def location(node):
        if node.location.file is None:
            return None
        name = node.location.file.name
        if name not in relative_paths:
            try:
                rel = str(Path(name).resolve().relative_to(repo))
                relative_paths[name] = rel if rel.startswith(("src/", "include/")) else None
            except ValueError:
                relative_paths[name] = None
        rel = relative_paths[name]
        if rel is None:
            return None
        return rel, node.location.offset

    for node in tu.cursor.get_children():
        if node.kind == cidx.CursorKind.MACRO_INSTANTIATION and node.referenced:
            site, definition = location(node), location(node.referenced)
            if site and definition in definitions:
                expansions[site].add(definition)
    visited_expansions = set()

    def walk(node, stack):
        site = location(node)
        if site and node.kind == cidx.CursorKind.DECL_REF_EXPR and node.referenced:
            definition = location(node.referenced)
            if definition in definitions:
                key, _label = semantic_context(cidx, node, stack)
                if key:
                    uses[definition].add(key)
        # Preprocessor cursors have no expression ancestors. Attach their
        # identities to the outermost expansion expression in the AST.
        if site in expansions and site not in visited_expansions and node.kind.is_expression():
            visited_expansions.add(site)
            if not _integral(cidx, node.type):
                role = "expanded macro has non-integral expression type"
            elif _evaluate_integer(cidx, node) is None and _runtime_initializer(cidx, node):
                role = "expanded macro expression depends on runtime values"
            else:
                role = ""
            if role:
                for definition in expansions[site]:
                    macro_roles[definition] = role
            key, _label = semantic_context(cidx, node, stack)
            if key:
                for definition in expansions[site]:
                    uses[definition].add(key)
        for child in node.get_children():
            walk(child, (*stack, node))

    walk(tu.cursor, ())
    return uses, macro_roles


def _evaluate_macros(cidx, path, args, included, repo, context):
    candidates = []
    rows = []
    for source in sorted(included):
        try:
            rel = str(source.relative_to(repo))
        except ValueError:
            continue
        if not rel.startswith(("src/", "include/")):
            continue
        for block in _named_blocks(source, repo):
            if block.kind != "define":
                continue
            member = block.members[0]
            expression = member.expression
            row = RawConstant(None, member.name, rel, member.line, member.column,
                              member.offset, "", "", -1, context, "define", expression)
            # Statement bodies, initializer lists and literal text are named
            # code/data macros, not scalar integer constants. Retain the reason.
            unquoted = re.sub(r"'(?:\\.|[^'\\])*'", "'character'", expression)
            if expression in {"override", "final", "const", "volatile", "inline", "__inline", "__forceinline", "__cdecl", "__stdcall", "__fastcall"}:
                rows.append(replace(row, status="excluded", reason="language modifier keyword macro, not a value expression"))
            elif any(token in unquoted for token in (";", "{", "}")) or expression.startswith(('"', 'L"')):
                rows.append(replace(row, status="excluded", reason="statement/aggregate/string macro, not an integral scalar"))
            else:
                candidates.append(row)
    if not candidates:
        return rows
    original = path.read_text(errors="replace")
    probe_lines = [f"\n__typeof__(({row.expression})) __hobbit_constant_{index} = ({row.expression});"
                   for index, row in enumerate(candidates)]
    probe = cidx.Index.create().parse(str(path), args=args,
        unsaved_files=[(str(path), original + "\n" + "\n".join(probe_lines))])
    evaluated = {node.spelling: node for node in probe.cursor.get_children()
                 if node.kind == cidx.CursorKind.VAR_DECL and node.spelling.startswith("__hobbit_constant_")}
    error_lines = {diag.location.line for diag in probe.diagnostics
                   if diag.severity >= cidx.Diagnostic.Error and diag.location.file
                   and Path(diag.location.file.name).resolve() == path.resolve()}
    for index, row in enumerate(candidates):
        node = evaluated.get(f"__hobbit_constant_{index}")
        if node is None or node.location.line in error_lines:
            rows.append(replace(row, status="unresolved", reason="macro replacement cannot be evaluated in including TU; requires explicit review"))
        elif not _integral(cidx, node.type):
            rows.append(replace(row, status="excluded", reason="macro replacement has non-integral type"))
        else:
            value = _evaluate_integer(cidx, node)
            rows.append(replace(row, value=value,
                status="evaluated" if value is not None else "unresolved",
                reason="" if value is not None else "macro replacement is not a compiler-evaluable integer"))
    return rows


def scan_entries(entries: list[dict], *, repo: Path = REPO, jobs: int = 1):
    payloads = [(entry, str(repo.resolve())) for entry in entries]
    rows = {}
    contexts = defaultdict(set)
    errors = []
    if jobs <= 1:
        results = map(_scan_entry, payloads)
        pool = None
    else:
        pool = ProcessPoolExecutor(max_workers=jobs)
        results = pool.map(_scan_entry, payloads)
    try:
        for constants, error in results:
            if error:
                errors.append(error)
                continue
            for constant in constants:
                key = (constant.file, constant.offset, constant.value, constant.status)
                old = rows.get(key)
                witness = old if old and old.reason.startswith("expanded macro") else constant
                rows[key] = replace(witness, use_contexts=tuple(sorted(
                    set(constant.use_contexts) | set(old.use_contexts if old else ()))))
                contexts[key].add(constant.context)
    finally:
        if pool is not None:
            pool.shutdown()
    # A macro may be included without being expanded in many TUs. An actual
    # non-value/runtime expansion is a witness for that same replacement;
    # do not preserve failures of the deliberately context-free probe from
    # TUs where it was never used. Conflicting evaluated values stay visible.
    witnessed_roles = {(row.file, row.offset, row.expression): row.reason
                       for row in rows.values() if row.kind == "define"
                       and row.status == "excluded" and row.reason.startswith("expanded macro")}
    for key, row in list(rows.items()):
        identity = row.file, row.offset, row.expression
        if row.kind == "define" and row.status == "unresolved" and identity in witnessed_roles:
            replacement = replace(row, status="excluded", reason=witnessed_roles[identity])
            new_key = (*key[:3], "excluded")
            rows[new_key] = replacement
            contexts[new_key].update(contexts[key])
            del rows[key]
    return rows, contexts, errors


def _join(raw, contexts, blocks):
    by_member = {}
    by_name = defaultdict(list)
    for block in blocks:
        for member in block.members:
            by_member[(block.file, member.offset)] = (block, member)
            by_name[(block.file, member.name)].append((block, member))

    constants = []
    uncovered_ast = []
    covered_members = set()
    for key, constant in raw.items():
        match = by_member.get((constant.file, constant.offset))
        if match is None:
            candidates = by_name.get((constant.file, constant.name), [])
            if len(candidates) == 1:
                match = candidates[0]
        if match is None and constant.kind != "enum":
            member = Member(constant.name, constant.line, constant.column,
                            constant.offset, constant.expression)
            # Function-local constants may share names. Location-qualified
            # domains keep those distinct without inventing a shared meaning.
            owner = constant.parent_name + "::" if constant.parent_name else ""
            domain = f"<{constant.kind}:{owner}{constant.name}>"
            identity = f"{constant.file}:{domain}"
            if any(block.source_enum == identity for block in blocks):
                identity += f"@{constant.line}:{constant.column}"
            block = Block(identity, domain, constant.kind, "", constant.file,
                          constant.line, constant.offset, constant.offset, (member,))
            blocks.append(block)
            match = block, member
            by_member[(constant.file, constant.offset)] = match
        if match is None:
            uncovered_ast.append(constant)
            continue
        block, member = match
        if constant.status == "unresolved":
            uncovered_ast.append(constant)
            continue
        covered_members.add((block.file, member.offset))
        if constant.status == "excluded":
            continue
        constants.append(Constant(
            constant.value, constant.name, constant.file, constant.line,
            constant.column, constant.offset, block.source_enum, block.domain,
            block.kind, block.storage, member.expression,
            tuple(sorted(contexts[key])), constant.use_contexts,
        ))

    uncovered_source = [
        (block, member)
        for block in blocks
        for member in block.members
        if (block.file, member.offset) not in covered_members
    ]
    constants.sort(key=lambda row: (row.value, row.file, row.offset))
    current_domains = {row.source_enum for row in constants}
    blocks[:] = [block for block in blocks if block.kind not in ("define", "const-integral")
                 or block.source_enum in current_domains
                 or any((block.file, member.offset) not in covered_members for member in block.members)]
    return constants, uncovered_source, uncovered_ast


def collect(*, cdb: Path = CDB, repo: Path = REPO, jobs: int = 1, with_coverage=False):
    before = source_stamp(repo)
    if not cdb.is_file():
        raise FileNotFoundError(f"{cdb}: no compile database; run hobbit configure")
    entries = json.loads(cdb.read_text())
    entries = [
        entry for entry in entries
        if Path(entry["file"]).suffix == ".cpp"
        and str(entry["file"]).replace("\\", "/").startswith("src/")
    ]
    if not entries:
        raise RuntimeError(f"{cdb}: no project C++ translation units")
    blocks = scan_blocks(repo=repo)
    raw, contexts, errors = scan_entries(entries, repo=repo, jobs=jobs)
    constants, uncovered_source, uncovered_ast = _join(raw, contexts, blocks)
    if source_stamp(repo) != before:
        errors.append("project source changed during the named-constant census; rerun on a stable tree")
    result = constants, blocks, uncovered_source, uncovered_ast, errors
    if with_coverage:
        coverage = [row for row in raw.values() if row.kind != "enum"]
        witnessed = {(row.file, row.offset) for row in coverage}
        for block, member in uncovered_source:
            if block.kind not in ("define", "const-integral") or (block.file, member.offset) in witnessed:
                continue
            coverage.append(RawConstant(None, member.name, block.file,
                member.line, member.column, member.offset, block.domain, block.file,
                block.offset, "", block.kind, member.expression, "unresolved",
                "source definition has no evaluated AST witness (inactive or unreferenced declaration)"))
        return (*result, coverage)
    return result


def _write_tsv(path: Path, fieldnames: tuple[str, ...], rows) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fieldnames, dialect="excel-tab",
                                lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)


def write_report(path: Path, constants: list[Constant]) -> None:
    fields = (
        "value", "hex", "name", "file", "line", "column", "offset",
        "source_enum", "domain", "kind", "storage", "expression", "contexts", "use_contexts",
    )
    rows = []
    for constant in constants:
        row = asdict(constant)
        row["hex"] = hex(constant.value)
        row["contexts"] = ";".join(constant.contexts)
        row["use_contexts"] = json.dumps(constant.use_contexts)
        rows.append({name: row[name] for name in fields})
    _write_tsv(path, fields, rows)


def _literal_counts(path: Path) -> dict[int, Counter]:
    result: dict[int, Counter] = defaultdict(Counter)
    if not path.is_file():
        return result
    with path.open(newline="") as stream:
        for row in csv.DictReader(stream, dialect="excel-tab"):
            if row["scope"] != "function-body" or row["value"] == "":
                continue
            value = int(row["value"])
            result[value][row["review_group"]] += 1
    return result


def write_collision_report(path: Path, constants: list[Constant], literals_path: Path) -> None:
    by_value = defaultdict(list)
    for constant in constants:
        by_value[constant.value].append(constant)
    literals = _literal_counts(literals_path)
    literal_contexts = defaultdict(set)
    if literals_path.is_file():
        with literals_path.open(newline="") as stream:
            for row in csv.DictReader(stream, dialect="excel-tab"):
                if row.get("value") and row.get("context_key"):
                    literal_contexts[int(row["value"])].add(row["context_key"])
    fields = (
        "value", "hex", "declarations", "domains", "members",
        "function_literal_sites", "literal_groups", "shared_named_contexts",
        "shared_literal_contexts",
    )
    rows = []
    for value, declarations in sorted(by_value.items()):
        domains = sorted({row.source_enum for row in declarations})
        literal_groups = literals.get(value, Counter())
        if len(domains) < 2 and not literal_groups:
            continue
        context_domains = defaultdict(set)
        for declaration in declarations:
            for key in declaration.use_contexts:
                context_domains[key].add(declaration.source_enum)
        rows.append({
            "value": value,
            "hex": hex(value),
            "declarations": len(declarations),
            "domains": ";".join(domains),
            "members": ";".join(f"{row.name}@{row.file}:{row.line}"
                                for row in declarations),
            "function_literal_sites": sum(literal_groups.values()),
            "literal_groups": ";".join(f"{name}={count}"
                                       for name, count in sorted(literal_groups.items())),
            "shared_named_contexts": json.dumps(sorted(key for key, owners in context_domains.items() if len(owners) > 1)),
            "shared_literal_contexts": json.dumps(sorted(set(context_domains) & literal_contexts[value])),
        })
    rows.sort(key=lambda row: (
        not json.loads(row["shared_named_contexts"]),
        not json.loads(row["shared_literal_contexts"]), row["value"]))
    _write_tsv(path, fields, rows)


def write_pair_report(path: Path, constants: list[Constant]) -> None:
    """Rank enum pairs by shared evaluated values without asserting identity."""
    by_domain = defaultdict(list)
    for constant in constants:
        by_domain[constant.source_enum].append(constant)
    fields = (
        "left", "right", "left_members", "right_members", "shared_values",
        "shared_count", "min_coverage_pct", "exact_value_set",
        "exact_value_sequence", "shared_contexts", "direct_shared_contexts",
    )
    rows = []
    for left_name, right_name in combinations(sorted(by_domain), 2):
        left = by_domain[left_name]
        right = by_domain[right_name]
        left_values = {row.value for row in left}
        right_values = {row.value for row in right}
        shared = sorted(left_values & right_values)
        exact_set = left_values == right_values
        if not shared:
            continue
        shared_contexts = set()
        for value in shared:
            left_contexts = {key for row in left if row.value == value for key in row.use_contexts}
            right_contexts = {key for row in right if row.value == value for key in row.use_contexts}
            shared_contexts.update(left_contexts & right_contexts)
        direct = sorted(key for key in shared_contexts if "/via:" not in key)
        coverage = 100.0 * len(shared) / min(len(left_values), len(right_values))
        rows.append({
            "left": left_name,
            "right": right_name,
            "left_members": ";".join(f"{row.name}={row.value}" for row in left),
            "right_members": ";".join(f"{row.name}={row.value}" for row in right),
            "shared_values": ";".join(str(value) for value in shared),
            "shared_count": len(shared),
            "min_coverage_pct": f"{coverage:.2f}",
            "exact_value_set": "yes" if exact_set else "no",
            "exact_value_sequence": (
                "yes" if [row.value for row in left] == [row.value for row in right]
                else "no"
            ),
            "shared_contexts": json.dumps(sorted(shared_contexts)),
            "direct_shared_contexts": json.dumps(direct),
        })
    rows.sort(key=lambda row: (
        -len(json.loads(row["direct_shared_contexts"])),
        -len(json.loads(row["shared_contexts"])),
        row["exact_value_sequence"] != "yes",
        row["exact_value_set"] != "yes",
        -float(row["min_coverage_pct"]),
        -row["shared_count"],
        row["left"],
        row["right"],
    ))
    _write_tsv(path, fields, rows)


def _members_by_enum(constants: list[Constant]) -> dict[str, list[Constant]]:
    result = defaultdict(list)
    for constant in constants:
        result[constant.source_enum].append(constant)
    for members in result.values():
        members.sort(key=lambda row: row.offset)
    return result


def init_ledger(path: Path, constants: list[Constant], blocks: list[Block]) -> None:
    """Snapshot the complete starting worklist; never overwrite review work."""
    if path.exists():
        raise FileExistsError(f"{path}: review ledger already exists")
    by_enum = _members_by_enum(constants)
    rows = []
    for block in blocks:
        members = by_enum.get(block.source_enum, ())
        rows.append({
            "source_enum": block.source_enum,
            "members": ";".join(f"{row.name}={row.value}" for row in members),
            "decision": "pending",
            "current_enums": block.source_enum,
            "member_reuse": "",
            "reason": "",
        })
    _write_tsv(path, LEDGER_FIELDS, rows)


def extend_ledger(path: Path, constants: list[Constant]) -> int:
    """Add unclaimed current declarations as pending, preserving old decisions."""
    with path.open(newline="") as stream:
        reader = csv.DictReader(stream, dialect="excel-tab")
        if tuple(reader.fieldnames or ()) != LEDGER_FIELDS:
            raise ValueError(f"{path}: unexpected ledger schema")
        rows = list(reader)
    claimed = set()
    existing = {row["source_enum"] for row in rows}
    for row in rows:
        members, findings = _parse_members(row["members"], source_enum=row["source_enum"])
        reuse, more = _parse_reuse(row["member_reuse"], source_enum=row["source_enum"])
        if findings or more:
            raise ValueError("; ".join(findings + more))
        claimed.update(reuse.get(name, f"{row['source_enum']}::{name}") for name in members)
    added = 0
    for domain, members in sorted(_members_by_enum(constants).items()):
        unclaimed = [row for row in members if f"{domain}::{row.name}" not in claimed]
        if not unclaimed:
            continue
        if domain in existing:
            # Preserve the starting snapshot and its reviewed decisions.
            # check_ledger still reports these unclaimed additions; they do
            # not prevent discovery of independent, wholly new domains.
            continue
        rows.append({"source_enum": domain,
                     "members": ";".join(f"{row.name}={row.value}" for row in unclaimed),
                     "decision": "pending", "current_enums": domain,
                     "member_reuse": "", "reason": ""})
        added += 1
    _write_tsv(path, LEDGER_FIELDS, rows)
    return added


def _parse_members(text: str, *, source_enum: str) -> tuple[dict[str, int], list[str]]:
    members = {}
    findings = []
    for item in filter(None, text.split(";")):
        if "=" not in item:
            findings.append(f"{source_enum}: malformed member snapshot {item!r}")
            continue
        name, value_text = item.rsplit("=", 1)
        try:
            value = int(value_text, 0)
        except ValueError:
            findings.append(f"{source_enum}: invalid value in snapshot {item!r}")
            continue
        if name in members:
            findings.append(f"{source_enum}: duplicate snapshot member {name}")
        members[name] = value
    return members, findings


def _parse_reuse(text: str, *, source_enum: str) -> tuple[dict[str, str], list[str]]:
    """Parse ``OLD=target-source-enum::TARGET`` member provenance entries."""
    reuse = {}
    findings = []
    for item in filter(None, text.split(";")):
        if "=" not in item:
            findings.append(f"{source_enum}: malformed member_reuse {item!r}")
            continue
        old, target = item.split("=", 1)
        if "::" not in target:
            findings.append(
                f"{source_enum}: member_reuse target needs source-enum::member: {item!r}")
            continue
        if old in reuse:
            findings.append(f"{source_enum}: duplicate member_reuse for {old}")
        reuse[old] = target
    return reuse, findings


def _code_tokens(text: str) -> tuple[str, ...]:
    code = _CHAR.sub(_blank_literal, _STRING.sub(_blank_literal, blank_comments(text)))
    return tuple(re.findall(r"[A-Za-z_]\w*|0[xX][\da-fA-F]+|\d+|[^\s]", code))


def _replacement_findings(repo: Path, replacements) -> list[str]:
    """Check replacement witnesses and reject surviving retired identifiers.

    The reason records the semantic review; token presence does not prove
    an SDK value or sizeof expression equals the historical snapshot.
    """
    if not replacements:
        return []
    sources = {
        str(path.relative_to(repo)): _code_tokens(path.read_text(errors="replace"))
        for root in (repo / "src", repo / "include")
        for path in root.rglob("*")
        if path.is_file() and path.suffix in _SOURCE_EXTENSIONS
    }
    identifiers = set(token for tokens in sources.values() for token in tokens)
    findings = []
    for owner, old, source, expression in replacements:
        if old in identifiers:
            findings.append(f"{owner}: replaced identifier {old} still occurs in source")
        tokens = sources.get(source)
        witness = _code_tokens(expression)
        if tokens is None:
            findings.append(f"{owner}: replacement source is not a project source file: {source}")
        elif not witness or not any(tokens[i:i + len(witness)] == witness
                                    for i in range(len(tokens) - len(witness) + 1)):
            findings.append(f"{owner}: replacement expression not found in {source}: {expression}")
    return findings


def check_ledger(path: Path, constants: list[Constant], *, repo: Path = REPO) -> list[str]:
    """Account for retained values, owner moves and reviewed expression replacements."""
    if not path.is_file():
        return [f"{path}: missing review ledger (run --init-ledger once)"]
    current = {
        f"{row.source_enum}::{row.name}": row.value
        for row in constants
    }
    claimed = set()
    findings = []
    seen = set()
    replacements = []
    with path.open(newline="") as stream:
        reader = csv.DictReader(stream, dialect="excel-tab")
        if tuple(reader.fieldnames or ()) != LEDGER_FIELDS:
            return [f"{path}: expected columns {','.join(LEDGER_FIELDS)}"]
        for line, row in enumerate(reader, 2):
            source_enum = row["source_enum"]
            if not source_enum:
                findings.append(f"{path}:{line}: empty source_enum")
                continue
            if source_enum in seen:
                findings.append(f"{path}:{line}: duplicate source_enum {source_enum}")
                continue
            seen.add(source_enum)
            decision = row["decision"]
            if decision not in LEDGER_DECISIONS:
                findings.append(
                    f"{source_enum}: invalid decision {decision!r}")
            if decision == "pending":
                findings.append(f"{source_enum}: review is pending")
            if decision != "pending" and not row["reason"].strip():
                findings.append(f"{source_enum}: reviewed row needs an evidence reason")
            members, row_findings = _parse_members(
                row["members"], source_enum=source_enum)
            findings.extend(row_findings)
            reuse, row_findings = _parse_reuse(
                row["member_reuse"], source_enum=source_enum)
            findings.extend(row_findings)
            unknown = sorted(set(reuse) - set(members))
            if unknown:
                findings.append(
                    f"{source_enum}: member_reuse names absent from snapshot: "
                    + ", ".join(unknown))
            if decision in ("reuse", "replace") and not reuse:
                findings.append(f"{source_enum}: {decision} decision has no member mapping")
            if decision in ("retain", "canonical") and reuse:
                findings.append(
                    f"{source_enum}: {decision} decision cannot redirect members")

            target_enums = set()
            expression_count = 0
            for name, value in members.items():
                target = reuse.get(name, f"{source_enum}::{name}")
                target_enum, separator, target_name = target.rpartition("::")
                if not separator or not target_enum or not target_name:
                    findings.append(
                        f"{source_enum}: invalid target for {name}: {target!r}")
                    continue
                if target_enum.startswith("expression:"):
                    expression_count += 1
                    if decision != "replace":
                        findings.append(f"{source_enum}: expression target requires replace decision")
                    if any(member.name == name for member in constants):
                        findings.append(f"{source_enum}: replaced member {name} still has a declaration")
                    replacements.append((source_enum, name,
                                         target_enum.removeprefix("expression:"), target_name))
                    continue
                target_enums.add(target_enum)
                if target not in current:
                    findings.append(
                        f"{source_enum}: {name} has no current target {target}")
                elif current[target] != value:
                    findings.append(
                        f"{source_enum}: {name}={value} maps to {target}="
                        f"{current[target]}")
                claimed.add(target)
            if decision == "replace" and not expression_count:
                findings.append(f"{source_enum}: replace decision has no expression target")
            declared_enums = set(filter(None, row["current_enums"].split(";")))
            if target_enums != declared_enums:
                findings.append(
                    f"{source_enum}: current_enums is {sorted(declared_enums)}, "
                    f"member targets use {sorted(target_enums)}")

    findings.extend(_replacement_findings(repo, replacements))
    unclaimed = sorted(set(current) - claimed)
    if unclaimed:
        preview = ", ".join(unclaimed[:10])
        suffix = " ..." if len(unclaimed) > 10 else ""
        findings.append(
            f"{len(unclaimed)} current member(s) have no starting-ledger provenance: "
            f"{preview}{suffix}")
    return findings


def _print_tsv(constants: list[Constant]) -> None:
    writer = csv.writer(sys.stdout, delimiter="\t", lineterminator="\n")
    writer.writerow(("value", "hex", "name", "file", "line", "domain", "kind"))
    for row in constants:
        writer.writerow((row.value, hex(row.value), row.name, row.file, row.line,
                         row.domain, row.kind))


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(
        prog="hobbit verify enum-reuse", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--value", action="append", type=lambda text: int(text, 0),
                        default=[], help="evaluated integer (repeatable)")
    parser.add_argument("--duplicates", action="store_true",
                        help="print only values declared in two or more domains")
    parser.add_argument("--json", action="store_true",
                        help="print the selected constants as JSON")
    parser.add_argument("--no-report", action="store_true",
                        help="do not write build/gen derived reports")
    parser.add_argument("--init-ledger", action="store_true",
                        help="create the complete pending review ledger; refuse overwrite")
    parser.add_argument("--extend-ledger", action="store_true",
                        help="append newly covered domains as pending; never approve them")
    parser.add_argument("--jobs", type=int,
                        default=min(4, multiprocessing.cpu_count()),
                        help="parallel libclang translation-unit workers")
    args = parser.parse_args(argv)
    try:
        constants, blocks, uncovered_source, uncovered_ast, errors, coverage = collect(
            jobs=max(1, args.jobs), with_coverage=True)
    except (FileNotFoundError, RuntimeError, ValueError) as exc:
        print(f"[enum-reuse] FATAL: {exc}")
        return 2
    if not args.no_report:
        _write_tsv(NAMED_REPORT,
            ("file", "line", "column", "name", "kind", "expression", "value", "status", "reason", "context"),
            ({name: asdict(row)[name] for name in
              ("file", "line", "column", "name", "kind", "expression", "value", "status", "reason", "context")}
             for row in sorted(coverage, key=lambda row: (row.file, row.offset, row.status))))
    incomplete = bool(errors or uncovered_source or uncovered_ast)
    if not args.no_report:
        outputs = (REPORT, COLLISION_REPORT, PAIR_REPORT)
        if incomplete:
            # Never leave a previous successful census at the canonical paths
            # while emitting a new partial one.
            for output in outputs:
                output.unlink(missing_ok=True)
            outputs = tuple(output.with_suffix(".partial.tsv") for output in outputs)
        write_report(outputs[0], constants)
        write_collision_report(outputs[1], constants, BARE_CONSTANTS)
        write_pair_report(outputs[2], constants)
        status_path = REPORT.with_name("enum_reuse_status.json")
        status_path.write_text(json.dumps({
            "complete": not incomplete, "evaluated_constants": len(constants),
            "parsing_errors": errors,
            "uncovered_source": len(uncovered_source),
            "uncovered_ast": len(uncovered_ast),
            "reports": [str(output.relative_to(REPO)) for output in outputs],
        }, indent=2) + "\n")
    if incomplete:
        for error in errors[:10]:
            print(f"   {error}")
        for block, member in uncovered_source[:10]:
            print(f"   {block.file}:{member.line}: source member not evaluated: "
                  f"{block.domain}::{member.name}")
        for row in uncovered_ast[:10]:
            print(f"   {row.file}:{row.line}: {row.reason or 'evaluated member not inventoried'}: "
                  f"{row.parent_name}::{row.name}")
        total = len(errors) + len(uncovered_source) + len(uncovered_ast)
        print(f"[enum-reuse] FATAL: {total} coverage/parsing finding(s)")
        print(f"[enum-reuse] partial inventory: {len(constants)} evaluated constants; "
              "see named_constant_coverage.tsv and enum_reuse_status.json")
        return 2

    by_value = Counter(row.value for row in constants)
    domains_by_value = defaultdict(set)
    for row in constants:
        domains_by_value[row.value].add(row.source_enum)
    selected = [
        row for row in constants
        if (not args.value or row.value in args.value)
        and (not args.duplicates or len(domains_by_value[row.value]) > 1)
    ]
    if args.json:
        print(json.dumps({
            "blocks": len(blocks),
            "constants": [asdict(row) for row in selected],
        }, indent=2))
    elif args.value or args.duplicates:
        _print_tsv(selected)
    if args.init_ledger:
        try:
            init_ledger(LEDGER, constants, blocks)
        except FileExistsError as exc:
            print(f"[enum-reuse] FATAL: {exc}")
            return 2
        print(f"[enum-reuse] initialized {LEDGER.relative_to(REPO)} with "
              f"{len(blocks)} pending row(s)", file=sys.stderr)
    if args.extend_ledger:
        try:
            count = extend_ledger(LEDGER, constants)
        except (FileNotFoundError, ValueError) as exc:
            print(f"[enum-reuse] FATAL: {exc}")
            return 2
        print(f"[enum-reuse] appended {count} pending domain(s)", file=sys.stderr)
    findings = check_ledger(LEDGER, constants)
    for finding in findings[:20]:
        print(f"   {finding}", file=sys.stderr)
    if len(findings) > 20:
        print(f"   ... {len(findings) - 20} more", file=sys.stderr)
    duplicate_values = sum(len(domains) > 1 for domains in domains_by_value.values())
    print(f"[enum-reuse] {len(blocks)} block(s), {len(constants)} member(s), "
          f"{len(by_value)} value(s), {duplicate_values} cross-domain collision value(s)",
          file=sys.stderr)
    if not args.no_report:
        print(f"[enum-reuse] reports: {REPORT.relative_to(REPO)}, "
              f"{COLLISION_REPORT.relative_to(REPO)}, "
              f"{PAIR_REPORT.relative_to(REPO)}", file=sys.stderr)
    if findings:
        print(f"[enum-reuse] FAIL: {len(findings)} ledger finding(s)", file=sys.stderr)
        return 1
    with LEDGER.open(newline="") as stream:
        reviewed = sum(1 for _row in csv.DictReader(stream, dialect="excel-tab"))
    print(f"[enum-reuse] OK: all {reviewed} starting block(s) reviewed and "
          "all current members accounted for", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
