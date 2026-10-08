"""hobbit.retail_labels.source - per-TU source-claim extraction.

    hobbit labels [--unit U ...] [--all] [-j N]

Per TU (ported from the old labels pipeline, mechanisms unchanged):

  RVA(rva, size)   clang IR: @llvm.global.annotations pairs the annotation
                   string DIRECTLY with the function's mangled symbol - no
                   positional join, so an inline header definition can never
                   steal a nearby address.
  DATA(rva)        clang drops extern annotations from IR, so the macro is
                   text-scanned (comments blanked) and bound to the AST
                   VarDecl BELOW it; the exact extent and the declaration's
                   linkage come from pylibclang.
  DATA_MESSAGE_MAP(map, entries)
                   annotates BEGIN_MESSAGE_MAP's GetMessageMap definition.
                   Its class scope selects the SDK's two actual VarDecls;
                   names, storage and extents still come from clang.
  RVA_COMPGEN      verbatim regex - the name is given, no join, no IR.
  RVA_DYNINIT      the `$E` owner pins - regex; the pin's owner stands in for
                   the volatile ordinal.
  DATA_COMPGEN     the compiler-generated DATUM pins. The macro expands to its
                   value expression, so no IR/AST carrier survives - it is
                   text-scanned (balanced parens: the macro sits in expression
                   position and clang-format wraps it) and the VALUE is the
                   claim: a narrow string literal is a pooled `??_C@` payload,
                   a float constant an FP-pool slot.

SPELLING. A claim's name is a function of SOURCE: clang proposes a mangled
name and `core.msvc_names` derives cl 5.0's own spelling from it (the i386
COFF prefix, the `@@<d>Q` -> `@@<d>P` array storage class, and the `$S`
TU-local wrapper whose per-object CodeView counter is dropped, exactly as
compare's canonicalization masks it on the object side). Objects may select otherwise unlabelled shared-header providers, but never
supply identities: LLVM confirms each current source annotation. A stale
build artifact can never answer for source that has
changed and a rule gap can never hide as a silent per-claim drop - the
corpus-wide control in `hobbit verify selftest` re-proves every rewrite
against the base objs once per build.

The one channel that still reads cl's object is DATA_COMPGEN, by doctrine: a
pin is admitted only when the TU's own base obj emitted that exact payload -
the pooled literal's `??_C@` name IS cl's spelling for those bytes, an FP
slot's `$T<n>` ordinal is volatile so the constant is proven by its bytes and
named for its rva; VC6 `__real@` symbols retain their proved value spelling
- AND the retail image holds those bytes at the pinned
address. A TU that compiles under cl but yields no IR is an ERROR: silently
contributing zero labels shrinks every denominator.

TUs without main-file macros are skipped unless a current object genuinely
emits an existing shared-header annotated identity. Such providers still
require normal LLVM source extraction; vendor library tables remain separate.
"""

from __future__ import annotations

import bisect
import os
import re
import struct

from hobbit.core import msvc_names
from hobbit.core.paths import BUILD, REPO
from hobbit.core.pe import image
from hobbit.core.tsv import write as write_tsv
from hobbit.retail_labels.fragments import FRAGMENTS, HEADER
from hobbit.manifest import units as manifest_units
from hobbit.tool import clang

#: cl's own objects. Only the DATA_COMPGEN channel reads them (its pin is
#: proven against the payload cl emitted); every other channel spells its
#: claims from source.
BASE_OBJS = BUILD / "objdiff/base"

# Presence test ONLY (never extraction): a TU with no rva.h macro at all is a
# vendored TU whose claims are the functions_zlib/data_zlib tables - skip it.
LABELED_TU_RE = re.compile(r"\b(?:RVA|DATA|RVA_COMPGEN|RVA_DYNINIT|DATA_COMPGEN|DATA_MESSAGE_MAP)\s*\(")
DATA_MACRO_RE = re.compile(r"\bDATA\s*\(\s*(0x[0-9a-fA-F]+)\s*\)")
DATA_MESSAGE_MAP_RE = re.compile(
    r"\bDATA_MESSAGE_MAP\s*\(\s*(0x[0-9a-fA-F]+)\s*,\s*(0x[0-9a-fA-F]+)\s*\)")
RVA_COMPGEN_RE = re.compile(
    r"\bRVA_COMPGEN\s*\(\s*(0x[0-9a-fA-F]+)\s*,\s*(0x[0-9a-fA-F]+|\d+)\s*,"
    r"\s*([^\s,)]+)\s*\)")
RVA_DYNINIT_RE = re.compile(
    r"\bRVA_DYNINIT\s*\(\s*(0x[0-9a-fA-F]+)\s*,\s*(0x[0-9a-fA-F]+|\d+)\s*,"
    r"\s*([A-Za-z_][A-Za-z0-9_:<>]*)\s*\)")
ANN_RVA_RE = re.compile(r"^rva:(0x[0-9a-fA-F]+)(?:\s+size:(0x[0-9a-fA-F]+|\d+))?$")
ANN_DATA_RE = re.compile(r"^data:(0x[0-9a-fA-F]+)$")
ANN_MESSAGE_MAP_RE = re.compile(
    r"^mfc-map:(0x[0-9a-fA-F]+) entries:(0x[0-9a-fA-F]+)$")

DATA_COMPGEN_RE = re.compile(r"\bDATA_COMPGEN\s*\(")
COMPGEN_ADDR_RE = re.compile(r"0x[0-9a-fA-F]{8}$")
_STR_SEG_RE = re.compile(r'"((?:[^"\\]|\\.)*)"')
_FLOAT_RE = re.compile(r"[-+]?(?:\d+\.\d*|\.\d+|\d+[eE][-+]?\d+"
                       r"|\d+\.?\d*[eE][-+]?\d+)([fF]?)$")
#: cl's floating-point literal pool member spelling. The `<n>` is a per-object
#: counter that renumbers on any TU churn, so it is never stored - the claim
#: names the slot for its rva, which is what the data manifest, the compgen
#: manifest and compare's content-addressing all spell.
FP_POOL_NAME = re.compile(r"^\$T[0-9]+$")
# Use the existing VC6 named-pool grammar and its downstream proof contract.
from hobbit.delink.real_constants import NAME as VC6_REAL_POOL_NAME

# @llvm.global.annotations tuple + the @.str constants it references.
_STR_DEF_RE = re.compile(r'^(@[\w.$"]+)\s*=.*?\bc"((?:[^"\\]|\\.)*)"', re.M)
_ANN_TUPLE_RE = re.compile(
    r'\{\s*ptr\s+(@(?:"[^"]+"|[\w.$]+))\s*,\s*ptr\s+(@(?:"[^"]+"|[\w.$]+))\s*,')


def _unescape_ir_cstr(s: str) -> str:
    out = bytearray()
    i = 0
    while i < len(s):
        if s[i] == "\\" and len(s) - i >= 3 and \
                all(c in "0123456789abcdefABCDEF" for c in s[i + 1:i + 3]):
            out.append(int(s[i + 1:i + 3], 16))
            i += 3
        else:
            out.append(ord(s[i]))
            i += 1
    if out and out[-1] == 0:
        out.pop()
    return out.decode("utf-8", "replace")


def _ir_symbol_name(ref: str) -> tuple[str, bool]:
    """`@"?Foo@@..."` / `@_foo` -> (bare symbol, already decorated).

    A `\\01` prefix means clang wrote the final object name itself; without it
    LLVM's i386 mangler still has to apply the COFF global prefix, which is
    what `core.msvc_names.decorate` reproduces."""
    ref = ref[1:]
    if ref.startswith('"') and ref.endswith('"'):
        ref = ref[1:-1]
    if ref.startswith("\\01"):
        return ref[3:], True
    return ref, False


#: `@name = [linkage ...] global|constant ...` - the storage a claim's spelling
#: depends on: cl wraps a global with no external linkage as `_<name>$S<n>`.
_IR_LINKAGE_RE = re.compile(
    r'^(@(?:"[^"]*"|[\w.$]+))\s*=\s*((?:[\w-]+\s+)*?)(?:global|constant)\b', re.M)


def ir_linkage(ir: str) -> dict[str, bool]:
    """{IR global reference: has internal linkage}."""
    return {m.group(1): "internal" in m.group(2).split()
            for m in _IR_LINKAGE_RE.finditer(ir)}


def ir_annotations(ir: str):
    """Yield (IR reference, symbol, already decorated, annotation) with the IR's direct join."""
    strings = {m.group(1): _unescape_ir_cstr(m.group(2))
               for m in _STR_DEF_RE.finditer(ir)}
    for line in ir.splitlines():
        if "@llvm.global.annotations" not in line:
            continue
        for sym_ref, str_ref in _ANN_TUPLE_RE.findall(line):
            if str_ref in strings:
                name, decorated = _ir_symbol_name(sym_ref)
                yield sym_ref, name, decorated, strings[str_ref]


def ir_claims(ir: str) -> tuple[list[tuple[int, str, int | None]],
                                list[tuple[int, str]]]:
    """(func, data) claims from @llvm.global.annotations, each name already in
    cl 5.0's spelling.

    Each annotation string arrives paired DIRECTLY with the symbol's mangled
    name. `data:` tuples cover every annotated DEFINITION; only extern-only
    declarations and constant-folded statics drop from IR and need the AST
    fallback."""
    linkage = ir_linkage(ir)
    funcs, datas = [], []
    for sym_ref, name, decorated, ann in ir_annotations(ir):
        m = ANN_RVA_RE.match(ann)
        if m:
            size = None
            if m.group(2):
                v = m.group(2)
                size = int(v, 16) if v.lower().startswith("0x") else int(v)
            funcs.append((int(m.group(1), 16),
                          msvc_names.func(name, decorated=decorated), size))
            continue
        m = ANN_DATA_RE.match(ann)
        if m:
            datas.append((int(m.group(1), 16),
                          msvc_names.data(name, decorated=decorated,
                                          internal=linkage.get(sym_ref, False))))
    return funcs, datas


def message_map_claims(ir: str, facts: dict) -> tuple[list[tuple], list[str]]:
    """Bind the two SDK-generated definitions, never invent mangled names.

    The annotation belongs to the generated getter, so another map or an
    intervening header inline cannot steal its data RVAs. Only static MFC's
    actual map and entry-array types are admitted; an absent/ambiguous member
    is fatal rather than a partial map claim.
    """
    out, problems = [], []
    for _ref, getter, _decorated, ann in ir_annotations(ir):
        pin = ANN_MESSAGE_MAP_RE.fullmatch(ann)
        if not pin:
            continue
        owner = re.fullmatch(r"\?GetMessageMap@(.+?)@@[EMU]BEPBUAFX_MSGMAP@@XZ", getter)
        if owner is None:
            problems.append(f"DATA_MESSAGE_MAP annotates {getter}, not an MFC GetMessageMap")
            continue
        scope = re.escape(owner[1])
        expected = (
            (int(pin[1], 16), rf"\?messageMap@{scope}@@[012]UAFX_MSGMAP@@B", 8),
            (int(pin[2], 16), rf"\?_messageEntries@{scope}@@[012]QBUAFX_MSGMAP_ENTRY@@B", None),
        )
        pair = []
        for rva, pattern, size in expected:
            candidates = [(name, fact) for name, fact in facts.items()
                          if re.fullmatch(pattern, name)]
            if len(candidates) != 1:
                problems.append(f"DATA_MESSAGE_MAP {getter}: expected one defined member "
                                f"matching {pattern}, found {len(candidates)}")
                continue
            name, fact = candidates[0]
            valid_size = (fact["size"] == size if size is not None
                          else fact["size"] >= 24 and fact["size"] % 24 == 0)
            if not fact.get("defined") or fact["internal"] or not valid_size:
                problems.append(f"DATA_MESSAGE_MAP {getter}: invalid definition/extent for {name}")
                continue
            pair.append((rva, fact["size"], msvc_names.data(
                name, decorated=True, internal=False)))
        if len(pair) == 2:
            out.extend(pair)
    return out, problems


def blank_comments(text: str) -> str:
    """`text` with // and /* */ bodies blanked (newlines kept) so a macro in
    a COMMENT is never read as a binding."""
    out = list(text)
    n, i, st = len(text), 0, "code"
    while i < n:
        c = text[i]
        if st == "code":
            if c == "/" and i + 1 < n and text[i + 1] == "/":
                while i < n and text[i] != "\n":
                    out[i] = " "
                    i += 1
                continue
            if c == "/" and i + 1 < n and text[i + 1] == "*":
                while i < n and not (text[i] == "*" and i + 1 < n
                                     and text[i + 1] == "/"):
                    if text[i] != "\n":
                        out[i] = " "
                    i += 1
                continue
            if c in "\"'":
                st = c
        elif c == "\\":
            i += 2
            continue
        elif c == st:
            st = "code"
        i += 1
    return "".join(out)


def _skip_quote(text: str, i: int) -> int:
    """Index just past the literal opening at `text[i]` (escapes honoured)."""
    quote, n = text[i], len(text)
    i += 1
    while i < n and text[i] != quote:
        i += 2 if text[i] == "\\" else 1
    return i + 1


def _split_top_level(body: str) -> list[str]:
    parts, depth, start, i = [], 0, 0, 0
    while i < len(body):
        c = body[i]
        if c in "\"'":
            i = _skip_quote(body, i)
            continue
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
        elif c == "," and depth == 0:
            parts.append(body[start:i])
            start = i + 1
        i += 1
    parts.append(body[start:])
    return [p.strip() for p in parts]


def compgen_invocations(text: str) -> list[tuple[int, list[str]]]:
    """[(line, [arg, ...])] for each DATA_COMPGEN(...) in blanked TU text.

    The macro sits in EXPRESSION position, so unlike the statement labels it
    may be wrapped by clang-format and two may share one line - the scan is
    balanced-paren and quote-aware, never line-based."""
    out = []
    for m in DATA_COMPGEN_RE.finditer(text):
        depth, j, n = 1, m.end(), len(text)
        while j < n and depth:
            c = text[j]
            if c in "\"'":
                j = _skip_quote(text, j)
                continue
            if c == "(":
                depth += 1
            elif c == ")":
                depth -= 1
            j += 1
        out.append((text.count("\n", 0, m.start()) + 1,
                    _split_top_level(text[m.end():j - 1])))
    return out


def compgen_value(value_src: str) -> tuple[str | None, bytes | str]:
    """('str'|'f32'|'f64', payload bytes), or (None, reason).

    The value SPELLING is the allocation's type: `0.0` is an f64 pool entry,
    `0.0f` an f32, a (concatenation of) narrow string literal(s) the pooled
    `??_C@` payload. Anything else - identifiers, wide strings, integers
    (immediates live in code, not data) - is rejected."""
    v = value_src.strip()
    if v.startswith('"'):
        segs = _STR_SEG_RE.findall(v)
        if not segs or _STR_SEG_RE.sub("", v).strip():
            return None, "value must be pure narrow string literal(s)"
        try:
            payload = ("".join(segs).encode("latin-1")
                       .decode("unicode_escape").encode("latin-1"))
        except (UnicodeDecodeError, UnicodeEncodeError):
            return None, "unsupported escape in string literal"
        return "str", payload
    m = _FLOAT_RE.fullmatch(v)
    if m:
        try:
            val = float(v[:-1] if m.group(1) else v)
        except ValueError:
            return None, "unparsable float constant"
        if m.group(1):
            return "f32", struct.pack("<f", val)
        return "f64", struct.pack("<d", val)
    return None, "value is neither a narrow string literal nor a float constant"


def obj_literals(obj_path) -> tuple[dict[bytes, str], dict[str, bytes]]:
    """({pooled payload: `??_C@` name}, {FP symbol: proven slot bytes}) of one base obj.

    The delink COFF reader is the tree's only section-payload parser; the
    name-authority reader (core.coff) answers names only, and duplicating the
    section walk here would fork the format knowledge."""
    from hobbit.delink.coffx import Obj
    c = Obj(obj_path)
    strings: dict[bytes, str] = {}
    for idx, value, secnum in c.iter_symbols():
        name = c.sym_name(idx)
        if name.startswith("??_C@") and secnum >= 1:
            payload = c.cstring(secnum, value)
            if payload is not None:
                strings.setdefault(payload, name)
    pool: dict[str, bytes] = {}
    for sec in c.section_table:
        members = c.section_members(sec["index"])
        slots = [(off, name) for off, name, _scl in members
                 if FP_POOL_NAME.fullmatch(name)]
        if not slots:
            continue
        starts = sorted(off for off, _n, _s in members)
        payload = c.section_payload(sec["index"])[:sec["size"]]
        for off, name in slots:      # the slot runs to the next member/section end
            end = next((o for o in starts if o > off), sec["size"])
            pool[name] = payload[off:end]
    # VC6 names an entire read-only Any-COMDAT float by its bit pattern.
    # Adapted from the local HoMM1 slot reader (revision
    # 3c2d9f32950d3198d1516c3dd279483ac8566baa), extending its $T rule with
    # actual Hobbit VC6 section/symbol/payload evidence. No arbitrary word scan.
    definitions = list(c.iter_symbols())
    counts = {}
    for idx, _off, sn in definitions:
        if sn > 0:
            name = c.sym_name(idx)
            counts[name] = counts.get(name, 0) + 1
    for idx, off, sn in definitions:
        name = c.sym_name(idx)
        match = VC6_REAL_POOL_NAME.fullmatch(name)
        if match is None or not (1 <= sn <= len(c.section_table)):
            continue
        sec = c.section_table[sn - 1]
        width = len(match.group(1)) // 2
        typ, storage = struct.unpack_from("<HB", c.buf, c.symptr + idx * 18 + 14)
        chars = sec["characteristics"]
        if (off != 0 or typ != 0 or storage != 2 or counts[name] != 1
                or sec["name"] != ".rdata" or sec["comdat"] != 2
                or not chars & 0x1000 or not chars & 0x40
                or not chars & 0x40000000 or chars & (0x80000000 | 0x20000000 | 0x80)
                or sec["reloc_count"] or sec["size"] != width
                or c.section_members(sn) != [(0, name, 2)]):
            continue
        payload = c.section_payload(sn)
        encoded = int(match.group(1), 16).to_bytes(width, "little")
        if len(payload) == width and payload == encoded:
            pool[name] = payload
    return strings, pool


def compgen_claims(text: str, unit: str, obj_path) -> tuple[list[tuple[int, str, int]],
                                                            list[str]]:
    """[(rva, name, size)] DATA_COMPGEN claims + [problems].

    Two independent facts per pin: the claiming TU's own base obj emitted that
    payload (cl's spelling for it is the name), and the retail image holds
    those bytes at the pinned address."""
    strings, pool = obj_literals(obj_path)
    img = image()
    claims, problems, seen = [], [], {}
    for line, args in compgen_invocations(text):
        where = f"{unit}: DATA_COMPGEN at line {line}"
        if len(args) != 2:
            problems.append(f"{where} takes (addr, value); got {len(args)} "
                            f"arg(s) (FATAL)")
            continue
        addr_src, value_src = args
        if not COMPGEN_ADDR_RE.fullmatch(addr_src):
            problems.append(f"{where}: address {addr_src!r} is not the "
                            f"canonical 8-digit 0x form (FATAL)")
            continue
        rva = int(addr_src, 16)
        vkind, payload = compgen_value(value_src)
        if vkind is None:
            problems.append(f"{where}: 0x{rva:06x} {payload} (FATAL)")
            continue
        if rva in seen:                       # repeated expansions coalesce
            if seen[rva] != (vkind, payload):
                problems.append(f"{where}: 0x{rva:06x} is claimed twice in "
                                f"this TU with different values (FATAL)")
            continue
        seen[rva] = (vkind, payload)
        if vkind == "str":
            name = strings.get(payload)
            if name is None:
                problems.append(f"{where}: 0x{rva:06x} {payload[:24]!r} is not "
                                f"a pooled ??_C@ literal in this TU's base obj "
                                f"(FATAL)")
                continue
            payload += b"\0"                  # the datum IS the NUL-terminated one
        else:
            # Named VC6 COMDATs must match their entire typed width/payload.
            named = [name for name, slot in pool.items()
                     if VC6_REAL_POOL_NAME.fullmatch(name) and slot == payload]
            if len(named) > 1:
                problems.append(f"{where}: 0x{rva:06x} ambiguous named floating "
                                f"COMDAT identity (FATAL)")
                continue
            # Preserve the donor's padded legacy $T slot contract separately.
            legacy = any(FP_POOL_NAME.fullmatch(name) and
                         slot[:len(payload)] == payload for name, slot in pool.items())
            if not named and not legacy:
                problems.append(f"{where}: 0x{rva:06x} {vkind} bits "
                                f"{payload.hex()} are in no proven floating pool slot of "
                                f"this TU's base obj (FATAL)")
                continue
            name = named[0] if named else f"$T{rva}"  # Only legacy ordinals are volatile.
        if img.read(rva, len(payload)) != payload:
            problems.append(f"{where}: 0x{rva:06x} retail bytes contradict the "
                            f"pinned value (FATAL)")
            continue
        claims.append((rva, name, len(payload)))
    return claims, problems


def _loc_file(loc) -> str | None:
    """clang's JSON printer omits `file` when unchanged, and a macro-expanded
    location carries it under expansionLoc/spellingLoc (expansion printed
    last). Reading only the top-level key silently loses every DATA() below a
    macro-expanded declaration - the measured BattlezMapConfig trap."""
    if not isinstance(loc, dict):
        return None
    for key in ("file", "expansionLoc", "spellingLoc"):
        v = loc.get(key)
        if key == "file":
            if v is not None:
                return v
        elif isinstance(v, dict) and v.get("file") is not None:
            return v["file"]
    return None


def collect_vars(ast: dict, main_file: str) -> list[tuple[str, int, str]]:
    """[(mangledName, offset, qualType)] for main-file global VarDecls."""
    main_real = os.path.realpath(main_file)
    out: list[tuple[str, int, str]] = []
    state = {"in_main": True}

    def visit(node):
        if not isinstance(node, dict):
            return
        for loc in (node.get("loc"), (node.get("range") or {}).get("begin")):
            f = _loc_file(loc)
            if f is not None:
                state["in_main"] = os.path.realpath(f) == main_real
        if (state["in_main"] and node.get("kind") == "VarDecl"
                and "mangledName" in node and not node.get("isImplicit")):
            off = (node.get("loc") or {}).get("offset")
            if off is not None:
                qt = (node.get("type") or {}).get("qualType") or ""
                out.append((node["mangledName"], off, qt))
        for c in node.get("inner") or []:
            visit(c)

    visit(ast)
    return out


def data_claims(text: str, ast: dict, main_file: str) -> list[tuple[int, str | None, str]]:
    """[(rva, mangledName|None, qualType)] - each DATA(0x..) bound to the
    VarDecl just BELOW it by line."""
    starts = [0]
    for i, ch in enumerate(text):
        if ch == "\n":
            starts.append(i + 1)
    var_defs = sorted((bisect.bisect_right(starts, off), mn, qt)
                      for (mn, off, qt) in collect_vars(ast, main_file))
    out = []
    for line_no, m in enumerate(blank_comments(text).splitlines(), 1):
        dm = DATA_MACRO_RE.search(m)
        if dm:
            cand = next(((mn, qt) for (dl, mn, qt) in var_defs if dl >= line_no),
                        (None, ""))
            out.append((int(dm.group(1), 16), cand[0], cand[1]))
    return out


def header_annotation_addresses() -> set[int]:
    """RVA annotation sites declared in shared source headers."""
    addresses = set()
    for path in (REPO / "include").rglob("*"):
        if path.suffix not in (".hpp", ".h") or path.name == "rva.h":
            continue
        text = blank_comments(path.read_text(errors="replace"))
        addresses.update(int(m.group(1), 16) for m in
                         re.finditer(r"\bRVA\s*\(\s*(0x[0-9a-fA-F]+)", text))
    return addresses



def header_annotation_names() -> set[str]:
    """Cached source names select candidates; LLVM supplies current claims."""
    from hobbit.retail_labels.fragments import all_claims
    addresses = header_annotation_addresses()
    return {c.name for c in all_claims()
            if c.kind == "func" and c.channel == "src" and c.rva in addresses}


def missing_header_annotations() -> set[int]:
    """An actual new header annotation may have no cached identity yet."""
    from hobbit.retail_labels.fragments import all_claims
    seen = {c.rva for c in all_claims() if c.kind == "func" and c.channel == "src"}
    return header_annotation_addresses() - seen


def emitted_executable_names(unit: str, source: str) -> set[str]:
    """Cold discovery presence filter; no object symbol creates a claim.

    As in the Gruntz donor, only the LLVM annotation join supplies identity.
    A cold header discovery joins those real names to current executable COFF
    definitions before writing a provider row. Cached names are unnecessary.
    """
    from hobbit.compare.canonicalize import CoffObject
    obj = BASE_OBJS / f"{unit}.obj"
    src = REPO / source
    if not obj.is_file() or obj.stat().st_mtime_ns < src.stat().st_mtime_ns:
        return set()
    coff = CoffObject(obj.read_bytes())
    return {s.name for s in coff.symbols.values()
            if s.section > 0 and s.typ & 0x20
            and coff.sections[s.section - 1].characteristics & 0x20000000}

def emitted_header_candidates(unit: str, source: str,
                              names: set[str]) -> set[str]:
    """COFF is only a presence filter for genuine unlabelled engine TUs.

    A missing or source-stale object cannot select a provider. Fresh LLVM
    source annotations, not the object or cache, supply every resulting claim.
    """
    from hobbit.compare.canonicalize import CoffObject
    obj = BASE_OBJS / f"{unit}.obj"
    src = REPO / source
    if not names or not obj.is_file() or obj.stat().st_mtime_ns < src.stat().st_mtime_ns:
        return set()
    coff = CoffObject(obj.read_bytes())
    return names.intersection(s.name for s in coff.symbols.values() if s.section > 0)


def extract_unit(unit: str, source: str, compdb: dict,
                 header_names: set[str] | None = None,
                 cold_headers: bool = False) -> tuple[list[list[str]], list[str]]:
    """(fragment rows, problems). Empty rows for a vendored (macro-free) TU."""
    src_path = REPO / source
    text = src_path.read_text(errors="replace")
    header_only = not LABELED_TU_RE.search(blank_comments(text))
    candidates = set()
    if header_only:
        candidates = emitted_header_candidates(unit, source,
                         header_annotation_names() if header_names is None else header_names)
        if cold_headers:
            candidates.update(emitted_executable_names(unit, source))
        if not candidates:
            return [], []

    problems: list[str] = []
    cl_flags = compdb.get(os.path.realpath(str(src_path)))
    rows: list[list[str]] = []

    def emit(rva, size, name, kind, channel, qtype=""):
        rows.append([f"0x{rva:08x}", f"0x{size:x}" if size is not None else "",
                     name, kind, channel, qtype])

    # functions via IR
    ir = clang.emit_ir(str(src_path), cl_flags)
    if ir is None:
        return rows, [f"{unit}: clang produced no IR - every RVA() label of "
                      f"this TU would silently vanish (FATAL)"]
    ir_funcs, ir_datas = ir_claims(ir)
    for rva, name, size in ir_funcs:
        if not header_only or name in candidates:
            emit(rva, size, name, "func", "src")
    if header_only:
        return rows, problems

    # compiler-generated bodies, name verbatim
    blanked = blank_comments(text)
    for m in RVA_COMPGEN_RE.finditer(blanked):
        rva, size = int(m.group(1), 16), int(m.group(2), 0)
        emit(rva, size or None, m.group(3), "func", "src_compgen")

    # $E dynamic-init owner pins (the body is a volatile ordinal by design)
    for m in RVA_DYNINIT_RE.finditer(blanked):
        emit(int(m.group(1), 16), int(m.group(2), 0) or None,
             m.group(3), "func", "src_dyninit")

    # compiler-generated DATA the automatic oracles cannot reach: the pinned
    # value is the claim, cl's own obj the authority (see the module header)
    if DATA_COMPGEN_RE.search(blanked):
        obj = BASE_OBJS / f"{unit}.obj"
        if not obj.is_file():
            problems.append(f"{unit}: DATA_COMPGEN pins cannot bind without "
                            f"{obj.name} - the pinned payload is proven "
                            f"against cl's own object (FATAL)")
        else:
            cg_claims, cg_problems = compgen_claims(blanked, unit, obj)
            problems.extend(cg_problems)
            for rva, name, size in cg_claims:
                emit(rva, size, name, "data", "src_data_compgen")

    # data: IR annotations primary (join-free), AST line-join only for the
    # declarations IR drops - an extern-only one, or a constant-folded static.
    # pylibclang gives every extent and, for that fallback, the linkage the
    # spelling depends on.
    if DATA_MACRO_RE.search(blanked) or DATA_MESSAGE_MAP_RE.search(blanked) or ir_datas:
        facts = clang.var_facts(str(src_path), cl_flags)
        if facts is None:
            problems.append(f"{unit}: pylibclang could not lay this TU out - "
                            f"every DATA() extent would vanish")
            facts = {}
        typed_facts = {msvc_names.data(name, decorated=True, internal=f["internal"]):
                       f for name, f in facts.items()}
        sizes = {name: fact["size"] for name, fact in typed_facts.items()}

        map_claims, map_problems = message_map_claims(ir, facts)
        problems.extend(f"{unit}: {p}" for p in map_problems)
        if len(map_claims) != 2 * len(DATA_MESSAGE_MAP_RE.findall(blanked)):
            problems.append(f"{unit}: DATA_MESSAGE_MAP did not bind both SDK data definitions")
        for rva, size, name in map_claims:
            emit(rva, size, name, "data", "src")

        covered = set()
        for rva, name in ir_datas:
            covered.add(rva)
            emit(rva, sizes.get(name), name, "data", "src",
                 typed_facts.get(name, {}).get("type", ""))
        site_count = len(DATA_MACRO_RE.findall(blanked))
        if site_count > len(covered):
            ast = clang.ast_dump(str(src_path), cl_flags)
            if ast is None:
                problems.append(f"{unit}: clang produced no AST - extern "
                                f"DATA() labels of this TU would vanish (FATAL)")
                return rows, problems
            for rva, mangled, qtype in data_claims(text, ast, str(src_path)):
                if rva in covered:
                    continue
                # both misses leave the site in NO fragment; the tree-wide
                # completeness sweep owns that FATAL (a per-unit run cannot
                # adjudicate whether another TU claims the same rva).
                if mangled is None:
                    problems.append(f"{unit}: DATA(0x{rva:06x}) has no VarDecl "
                                    f"below it")
                    continue
                fact = facts.get(mangled)
                if fact is None:
                    problems.append(f"{unit}: DATA(0x{rva:06x}) {mangled} has "
                                    f"no pylibclang declaration - its storage, "
                                    f"and so cl's spelling, is unknown")
                    continue
                name = msvc_names.data(mangled, decorated=True,
                                       internal=fact["internal"])
                emit(rva, fact["size"], name, "data", "src", fact.get("type", qtype))

    return rows, problems


MACRO_SITE_RE = re.compile(
    r"\b(RVA_COMPGEN|RVA_DYNINIT|DATA_COMPGEN|RVA|DATA)\s*\(\s*(0x[0-9a-fA-F]+)")


def sweep_sites() -> dict[str, dict[int, str]]:
    """Tree-wide macro-site census over src/ + include/ (comments blanked,
    rva.h's own #defines excluded): {macro: {rva: 'file:line'}}.

    The completeness oracle: every site must be accounted for by a fragment
    or a stated doctrine rule - a macro that neither extraction nor doctrine
    reaches is a silently lost label, the old tree's worst failure class."""
    out: dict[str, dict[int, list[str]]] = {}
    for base in ("src", "include"):
        root = REPO / base
        if not root.is_dir():
            continue
        for path in sorted(root.rglob("*")):
            if path.suffix not in (".cpp", ".c", ".hpp", ".h") or path.name == "rva.h":
                continue
            text = blank_comments(path.read_text(errors="replace"))
            for m in MACRO_SITE_RE.finditer(text):        # whole-file: a macro
                lineno = text.count("\n", 0, m.start()) + 1   # may span lines
                out.setdefault(m.group(1), {}).setdefault(
                    int(m.group(2), 16), []).append(
                    f"{path.relative_to(REPO)}:{lineno}")
            for m in DATA_MESSAGE_MAP_RE.finditer(text):
                lineno = text.count("\n", 0, m.start()) + 1
                for addr in m.groups():
                    out.setdefault("DATA_MESSAGE_MAP", {}).setdefault(
                        int(addr, 16), []).append(f"{path.relative_to(REPO)}:{lineno}")
    return out


def check_completeness() -> list[str]:
    """The oracle over extraction: every site accounted for, loudly.

    A site's claim may be carried by any unit's fragment - a header inline's
    macro reaches every including TU - so the sweep asks only whether the
    (channel, kind, rva) exists at all."""
    from hobbit.retail_labels.fragments import all_claims
    sites = sweep_sites()
    have: dict[tuple[str, str], set[int]] = {}
    for c in all_claims():
        have.setdefault((c.channel, c.kind), set()).add(c.rva)
    problems = []
    checks = [("RVA", "src", "func"), ("RVA_COMPGEN", "src_compgen", "func"),
              ("RVA_DYNINIT", "src_dyninit", "func"), ("DATA", "src", "data"),
              ("DATA_COMPGEN", "src_data_compgen", "data")]
    checks.append(("DATA_MESSAGE_MAP", "src", "data"))
    for macro, channel, kind in checks:
        for rva, wheres in sorted(sites.get(macro, {}).items()):
            if macro == "DATA" and all(".h:" in w for w in wheres):
                problems.append(f"DATA(0x{rva:06x}) at {wheres[0]} is in a "
                                f"HEADER - extraction ignores it by design; a "
                                f"header static belongs in data_compgen.tsv "
                                f"(FATAL)")
                continue
            if rva not in have.get((channel, kind), set()):
                problems.append(f"{macro}(0x{rva:06x}) at {wheres[0]} is in "
                                f"NO fragment - silently lost label (FATAL)")
            if len(wheres) > 1 and macro != "RVA":
                problems.append(f"{macro}(0x{rva:06x}) appears at "
                                f"{len(wheres)} sites ({wheres[0]} ...) - "
                                f"stacked/duplicated macro")
    return problems


def run(only_units: list[str] | None = None, jobs: int = os.cpu_count() or 4):
    """Extract fragments; returns (changed, problems). Fragment writes are
    content-idempotent so unchanged TUs never dirty downstream edges."""
    from concurrent.futures import ThreadPoolExecutor

    db = clang.compdb()
    units = manifest_units()
    if only_units is not None:
        known = {u["unit"] for u in units}
        for name in only_units:
            if name not in known:
                stem = os.path.splitext(os.path.basename(name))[0]
                stems = {stem.lower(), name.lower()}
                hint = sorted(k for k in known if k in stems)
                raise SystemExit(
                    f"[extract] unknown unit {name!r}"
                    + (f" - did you mean {hint[0]!r}?" if hint else
                       " - units are manifest stems, e.g. 'cimage'"))
    todo = [u for u in units
            if only_units is None or u["unit"] in only_units]
    changed, problems = [], []
    header_names = header_annotation_names()
    cold_headers = bool(missing_header_annotations())

    def one(u):
        rows, probs = extract_unit(u["unit"], u["source"], db, header_names, cold_headers)
        if any(isinstance(pr, str) and "FATAL" in pr for pr in probs):
            # never replace a good cached fragment with a truncated one
            return u["unit"], None, probs
        banner = [f"# GENERATED claim fragment for unit {u['unit']} - the "
                  f"macros in {u['source']} are the storage; do not edit."]
        did = write_tsv(FRAGMENTS / f"{u['unit']}.tsv", banner, HEADER, rows)
        return u["unit"], did, probs

    with ThreadPoolExecutor(max_workers=jobs) as pool:
        for unit, did, probs in pool.map(one, todo):
            if did:
                changed.append(unit)
            problems.extend(probs)
    if only_units is None:
        problems.extend(check_completeness())
    return changed, problems


def main() -> int:
    import argparse
    import sys
    ap = argparse.ArgumentParser(
        prog="hobbit labels", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--unit", action="append", help="extract one unit (repeatable)")
    ap.add_argument("--all", action="store_true",
                    help="extract every unit in the census")
    ap.add_argument("-j", "--jobs", type=int, default=os.cpu_count() or 4)
    a = ap.parse_args()
    if not a.unit and not a.all:
        ap.error("pick --unit U or --all")
    changed, problems = run(a.unit if not a.all else None, a.jobs)
    for p in problems:
        print(f"[extract] {p}", file=sys.stderr)
    print(f"[extract] {len(changed)} fragment(s) changed")
    return 1 if any("FATAL" in p for p in problems) else 0


if __name__ == "__main__":
    raise SystemExit(main())
