"""hobbit.delink.pdb_synth - synthesize the delinker's named PDB from the Model.

No original Hobbit game PDB has been established; this builds analysis metadata
for vostok-delinker to slice the hash-pinned Meridian.exe into per-unit COFF objects:

  1. Function records from the Model's function bindings: the claimed name (or
     a FUN_<va> placeholder), the claim-resolved extent, and the owning unit as
     a synthetic `c:\\proj\\<unit>.c` source file (C13 line info) so the
     delinker emits one `<unit>.c.obj` per TU. kind=pad rows are never emitted;
     kind=eh rows are superseded by the per-owner EH-band records
     (hobbit.delink.eh_band). ILT/import thunks inherit the body/import name so
     relocations pair by name on both sides.
  2. Data records for every relocation-target address (S_LDATA32), renamed to
     the claimed source names and cl's own `??_C@` string-pool spellings (the
     base objs are the oracle), plus the proven `__imp_` IAT decorations.
     Every identity is PROVIDED, never invented: a target no name reaches
     keeps a fence whose spelling states the verdict of the referencing-band
     split - `DAT_<va>` when only library bands reference it (a deliberate
     synthetic), `UNPROVISIONED_<va>` when any game band does (a defect the
     delinker refuses to emit).
  3. `llvm-pdbutil yaml2pdb`, then the DBI-header byte-patch: yaml2pdb cannot
     emit a GSI symbol-records stream and pdb2's `global_symbols()` errors on a
     nil index, so the header is repointed at an existing empty stream.
"""

from __future__ import annotations

import bisect
import hashlib
import struct
from pathlib import Path

from hobbit.core.paths import BUILD
from hobbit.delink import coffx, dyninit, eh_band, implib, static_dtors
from hobbit.delink.image import retail, sections_of
from hobbit.model import Model

PDB_DIR = BUILD / "pdb"
BASE_DIR = BUILD / "objdiff/base"
MODULE_PATH = r"c:\proj\hobbit.obj"

#: Functions with no claimed unit fall into address buckets of 2**BUCKET_SHIFT
#: bytes (the granularity the matched functions were delinked under).
BUCKET_SHIFT = 16

#: Function channels that attribute a unit (a static-lib label names a library
#: body; its code is never partitioned into a TU).
UNIT_CHANNELS = ("src", "src_compgen", "src_dyninit", "functions_zlib")

#: The two fence spellings reloc_data_symbols seeds for an unnamed target.
FENCE_PREFIXES = ("DAT_", "UNPROVISIONED_")


def sanitize_name(name: str) -> str:
    """Make a symbol name safe to embed in single-quoted YAML (yaml2pdb takes
    the DisplayName verbatim into the CodeView record)."""
    cleaned = "".join(c if (32 <= ord(c) < 127) else "_" for c in name)
    return cleaned.replace("'", "''")


# --- name derivation from the Model -----------------------------------------

def unit_names(model: Model) -> dict[int, tuple[str, str, int]]:
    """{rva: (name, unit, size)} for the unit-attributed function claims.

    A NAMELESS src_dyninit pin stays out here: its volatile `_$E<n>` ordinal
    is derived from the current base object only after the owner is proved."""
    return {b.rva: (b.name, b.unit, b.size) for b in model.functions
            if b.channel in UNIT_CHANNELS and b.name}


def ilt_thunk_names(model: Model, names_map: dict) -> dict[int, str]:
    """Map retail ILT forwarding thunks to curated body names.

    A census row whose bytes are a 5-byte `E9 rel32` (any kind=thunk row, plus
    any derived-size-5 row) inherits a name only when its exact target RVA is
    already in `names_map`; chains resolve to a fixpoint. No presumed
    Gruntz ILT address band is scanned in this target.
    """
    img = retail()
    text = img.pe.section(".text")
    text_lo, raw_size, raw_ptr = text["va"], text["rsize"], text["rptr"]

    e9: dict[int, int] = {}
    for b in model.functions:
        if b.size != 5 or b.kind != "thunk":
            continue
        off = raw_ptr + b.rva - text_lo
        if not raw_ptr <= off <= raw_ptr + raw_size - 5:
            continue
        if img.data[off] != 0xE9:
            continue
        e9[b.rva] = b.rva + 5 + struct.unpack_from("<i", img.data, off + 1)[0]

    aliases: dict[int, str] = {}
    while True:
        grew = False
        for rva, target in e9.items():
            if rva in aliases or rva in names_map:
                continue
            name = names_map[target][0] if target in names_map \
                else aliases.get(target)
            if name is not None:
                aliases[rva] = name
                grew = True
        if not grew:
            break

    return aliases


def import_thunk_names(iat_syms, names_map: dict) -> dict[int, str]:
    """{rva: `_Foo@N`} for every 6-byte `FF 25 <IAT slot>` import thunk.

    MSVC compiles a non-dllimport Win32 call as `call rel32`; the linker
    plants `jmp DWORD PTR [__imp__Foo@N]` and points the call at it. The base
    obj relocates against `_Foo@N` directly, so the thunk must carry that name
    or the same call compares unequal against an anonymous FUN_<va>. Derived
    only from IAT slots with a PROVEN `__imp_` decoration.
    """
    slots = {}
    for slot, dec in iat_syms:
        slots[slot] = "_" + dec[len("__imp__"):] if dec.startswith("__imp__") \
            else dec[len("__imp_"):]
    img = retail()
    from hobbit.retail_labels.censuses import functions
    out = {}
    for row in functions():
        if row["kind"] != "thunk" or row["size"] != 6:
            continue
        rva = row["rva"]
        code = img.raw_read(rva, 6)
        if code is None or code[:2] != b"\xff\x25":
            continue
        target = struct.unpack_from("<I", code, 2)[0] - img.image_base
        if target in slots and rva not in names_map:
            out[rva] = slots[target]
    return out


def function_records(model: Model, names_map, thunk_names, import_thunks,
                     drop_spans, log) -> list[tuple[int, int, str]]:
    """(rva, size, name) for every emitted function record, sorted.

    kind=pad rows never emit; rows inside `drop_spans` (the EH funclet band)
    are superseded by the per-owner records already merged into `names_map`.
    """
    image_base = retail().image_base
    spans = sorted(drop_spans)
    span_lo = [lo for lo, _hi in spans]

    def superseded(rva: int) -> bool:
        index = bisect.bisect_right(span_lo, rva) - 1
        return index >= 0 and rva < spans[index][1]

    out, seen, dropped = [], set(), 0
    for b in model.functions:
        if b.size <= 0 or b.kind in ("pad", "unknown"):
            continue
        if superseded(b.rva):
            dropped += 1
            continue
        size = b.size
        if b.rva in names_map:
            name = names_map[b.rva][0]
            at_size = names_map[b.rva][2]
            if at_size:
                size = min(at_size, size)
        elif b.rva in thunk_names:
            name = thunk_names[b.rva]
        elif b.rva in import_thunks:
            name = import_thunks[b.rva]
        else:
            name = f"FUN_{image_base + b.rva:08x}"
        out.append((b.rva, size, name))
        seen.add(b.rva)
    if dropped:
        log(f"superseded {dropped} census row(s) inside the EH funclet band")

    # A band thunk absent from the census still needs a record so its callers'
    # relocations resolve to the curated name (exactly the 5-byte `E9 rel32`).
    synth_thunks = 0
    for rva, name in sorted(thunk_names.items()):
        if rva not in seen:
            out.append((rva, 5, name))
            seen.add(rva)
            synth_thunks += 1
    if synth_thunks:
        log(f"synthesized {synth_thunks} ILT thunk record(s) absent from the census")

    synth_imp = 0
    for rva, name in sorted(import_thunks.items()):
        if rva not in seen:
            out.append((rva, 6, name))
            seen.add(rva)
            synth_imp += 1
    if synth_imp:
        log(f"synthesized {synth_imp} import thunk record(s) from named IAT slots")

    # names_map entries absent from the census: the EH-band per-owner records.
    synth = 0
    for rva, (name, _unit, size) in sorted(names_map.items()):
        if rva in seen or size <= 0:
            continue
        out.append((rva, size, name))
        seen.add(rva)
        synth += 1
    if synth:
        log(f"synthesized {synth} record(s) at RVAs the census never carved")
    out.sort()
    return out


def folded_alias_records(model: Model, funcs) -> list[tuple[int, int, str, str]]:
    """Retain admitted source aliases with their full shared extent and owner.

    Binding.aliases includes dynamic-initializer DATA owner names; those are
    not procedure identities. Only src/src_compgen function claims qualify.
    Each alias is emitted in a separate module so its C13 line ownership does
    not collapse onto the winning name's source file at the same offset.
    """
    selected = set(funcs)
    out = set()
    for b in model.functions:
        if (b.rva, b.size, b.name) not in selected:
            continue
        for a in b.aliases:
            if a.channel not in ("src", "src_compgen") or a.kind != "func":
                continue
            if not a.name or a.name == b.name:
                continue
            if a.rva != b.rva or a.size != b.size or b.size <= 0 or not a.unit:
                raise ValueError(f"folded function alias {a.name!r} at {b.rva:#x} "
                                 "requires the same full extent and a source owner")
            out.add((a.rva, a.size, a.name, a.unit))
    return sorted(out)


# --- data symbols ------------------------------------------------------------

def reloc_target_refs() -> dict[int, list[int]]:
    """{target rva in .rdata/.data: sorted [reloc site rva]} - every absolute
    address operand in the PE's HIGHLOW relocation directory, keyed by the
    exact target its stored value identifies."""
    img = retail()
    bounds = sections_of()
    rd_lo, rd_hi = bounds[".rdata"]
    da_lo, da_hi = bounds[".data"]
    refs: dict[int, list[int]] = {}
    iat_slots = {slot for slot, _name, _dll, _ordinal in img.import_slots()}
    for site in img.absolute_sites:
        value = img.u32(site)
        if value is None:
            continue
        rva = value - img.image_base
        if rva in iat_slots:
            continue
        if rd_lo <= rva < rd_hi or da_lo <= rva < da_hi:
            refs.setdefault(rva, []).append(site)
    return refs


def band_lookup():
    """rva -> link-layout band name (config/retail/link_bands.tsv), '' when
    outside every band."""
    from hobbit.retail_labels.censuses import link_bands
    bands = link_bands()
    lows = [lo for lo, _hi, _name in bands]

    def of(rva: int) -> str:
        i = bisect.bisect_right(lows, rva) - 1
        if i >= 0 and rva < bands[i][1]:
            return bands[i][2]
        return ""
    return of


def game_site_test(model: Model):
    """Unknown ownership is unprovisioned; only a proved library body may
    exempt a reference. Hobbit has no admitted Gruntz-style link bands."""
    lib = sorted((b.rva, b.size) for b in model.functions
                 if b.channel == "functions_static_libs" and b.size)
    lib_starts = [x[0] for x in lib]

    def is_game(site: int) -> bool:
        i = bisect.bisect_right(lib_starts, site) - 1
        return not (i >= 0 and site < lib[i][0] + lib[i][1])
    return is_game


def reloc_data_symbols(model: Model) -> tuple[list, list]:
    """(rdata, data) [(rva, fence name)] for every PE relocation target.

    Source names are overlaid later; a target no name reaches keeps the fence
    seeded HERE, spelled by the referencing split (game_site_test): `DAT_<va>`
    when only library code references it (library-internal data we
    deliberately do not model - made explicit and total), `UNPROVISIONED_<va>`
    when any game site references it. The fence must EXIST either way so
    nearest-symbol recovery can never silently misattribute the address; the
    delinker emits `DAT_` but hard-fails on `UNPROVISIONED_`.
    """
    img = retail()
    rd_lo, rd_hi = sections_of()[".rdata"]
    is_game = game_site_test(model)
    rdata: list[tuple[int, str]] = []
    data: list[tuple[int, str]] = []
    for rva, sites in sorted(reloc_target_refs().items()):
        prefix = "UNPROVISIONED" if any(map(is_game, sites)) else "DAT"
        row = (rva, f"{prefix}_{img.image_base + rva:08x}")
        (rdata if rd_lo <= rva < rd_hi else data).append(row)
    return rdata, data


def apply_named_data(rdata_syms, data_syms, data_names) -> int:
    """Overlay the claimed names onto the DAT_ placeholders (adding a symbol
    when the address is not a relocation target). Mutates; returns count."""
    bounds = sections_of()
    n = 0
    for syms, (lo, hi) in ((rdata_syms, bounds[".rdata"]),
                           (data_syms, bounds[".data"])):
        seen = {rva for rva, _ in syms}
        for i, (rva, _name) in enumerate(syms):
            if rva in data_names:
                syms[i] = (rva, data_names[rva])
                n += 1
        for rva, name in data_names.items():
            if rva not in seen and lo <= rva < hi:
                syms.append((rva, name))
                n += 1
    rdata_syms.sort()
    data_syms.sort()
    return n


def drop_interior_placeholders(rdata_syms, data_syms, model) -> int:
    """A fence at an address a claimed extent CONTAINS is not an unprovided
    identity - the containing claim + addend is, and keeping the fence would
    shadow that claim in nearest-symbol selection. Drop them (both fence
    spellings). An `UNPROVISIONED_` that survives this marks a genuinely
    unprovided game-referenced address, which the delinker refuses to emit.
    Mutates; returns count."""
    claims = sorted((b.rva, b.size) for b in model.data
                    if b.channel and b.name and b.size)
    starts = [c[0] for c in claims]

    def interior(rva: int) -> bool:
        i = bisect.bisect_right(starts, rva) - 1
        return i >= 0 and claims[i][0] < rva < claims[i][0] + claims[i][1]

    n = 0
    for syms in (rdata_syms, data_syms):
        keep = [(rva, name) for rva, name in syms
                if not (name.startswith(FENCE_PREFIXES) and interior(rva))]
        n += len(syms) - len(keep)
        syms[:] = keep
    return n


def apply_string_names(rdata_syms, data_syms, base_dir) -> int:
    """Rename string-constant symbols to their MSVC `??_C@` pool names.

    cl names a pooled literal by length + a compiler-generated checksum + the text; we
    never recompute it - the base objects ARE cl output, so their `??_C@`
    symbols give the exact name for each literal's bytes. Mutates; returns the
    count renamed.
    """
    str_map = coffx.build_string_map(Path(base_dir))
    if not str_map:
        return 0
    img = retail()
    n = 0
    for syms in (rdata_syms, data_syms):
        for i, (rva, name) in enumerate(syms):
            if name.startswith("??_C@"):
                continue
            cs = img.cstring(rva)
            if cs and cs in str_map:
                syms[i] = (rva, str_map[cs])
                n += 1
    return n


def data_symbols(model, data_names, base_dir=BASE_DIR, log=lambda m: None):
    """The fully-overlaid (rdata_syms, data_syms): fences for every reloc
    target, claimed names applied, `??_C@` pool spellings applied, interior
    fences dropped. The data-identity half of the synthesis, shared with the
    --unprovisioned worklist."""
    rdata_syms, data_syms = reloc_data_symbols(model)
    ndat = apply_named_data(rdata_syms, data_syms, data_names)
    log(f"named {ndat} global data symbol(s) from the Model")
    nstr = apply_string_names(rdata_syms, data_syms, base_dir)
    log(f"renamed {nstr} string constant(s) to MSVC ??_C@ names")
    ndrop = drop_interior_placeholders(rdata_syms, data_syms, model)
    log(f"dropped {ndrop} interior fence(s) (contained by a claim)")
    return rdata_syms, data_syms


# --- the unprovisioned worklist ----------------------------------------------

def _oracle_extents(model) -> list[tuple[int, int]]:
    """[(rva, size)] the data manifest provides WITHOUT a Model claim (the
    `??_C@` string and `$T` FP-pool oracles): the delinker resolves those
    extents before its PDB fallback, so a fence inside one never fires."""
    from hobbit.delink import data_manifest
    rows = data_manifest.string_rows()[0] + data_manifest.fp_pool_rows(model)[0]
    from hobbit.delink.empty_strings import rows as empty_string_rows
    rows += empty_string_rows(model, retail(), BASE_DIR)[0]
    from hobbit.delink.private_strings import rows as private_string_rows
    rows += private_string_rows(model, retail(), BASE_DIR)[0]
    from hobbit.delink.seh_tables import rows as seh_table_rows
    rows += seh_table_rows(model, retail(), BASE_DIR)[0]
    from hobbit.delink.static_guards import rows as static_guard_rows
    rows += static_guard_rows(model, retail(), BASE_DIR)[0]
    from hobbit.delink.real_constants import rows as real_constant_rows
    rows += real_constant_rows(model, retail(), BASE_DIR)[0]
    return sorted({(r["rva"], r["size"]) for r in rows})


def _alias_covered(model):
    """site-coverage test from the reviewed reloc-alias manifest
    (config/retail/reloc_referents.tsv): the delinker resolves a covered site
    BEFORE its PDB fallback, so the fence never fires for it."""
    from hobbit.core.paths import RETAIL
    from hobbit.core.tsv import read as read_tsv
    _b, _h, raw = read_tsv(RETAIL / "reloc_referents.tsv")
    ext = {b.rva: b.size for b in model.functions if b.channel and b.size}
    exact: set[tuple[int, int]] = set()
    wild: dict[int, list[tuple[int, int]]] = {}
    for r in raw:
        target = int(r["target_rva"], 16)
        fn = int(r["function_rva"], 16)
        if r["site_rva"].strip() == "*":
            wild.setdefault(target, []).append((fn, fn + ext.get(fn, 0)))
        else:
            exact.add((target, int(r["site_rva"], 16)))

    def covered(target: int, site: int) -> bool:
        if (target, site) in exact:
            return True
        return any(lo <= site < hi for lo, hi in wild.get(target, ()))
    return covered


def unprovisioned_rows(rdata_syms, data_syms, model) -> list[dict]:
    """The remaining-UNPROVISIONED worklist, DERIVED every build (never a
    hand-kept file): each surviving `UNPROVISIONED_` fence, minus the
    oracle-provided extents and minus targets whose every game site the
    reloc-alias manifest covers, with the uncovered game sites, their bands,
    the referencing claimed units, and the admitted census row containing it."""
    from hobbit.retail_labels import censuses
    fences = sorted(rva for syms in (rdata_syms, data_syms)
                    for rva, name in syms if name.startswith("UNPROVISIONED_"))
    if not fences:
        return []
    oracle = _oracle_extents(model)
    oracle_starts = [lo for lo, _sz in oracle]

    def oracled(rva: int) -> bool:
        i = bisect.bisect_right(oracle_starts, rva) - 1
        return i >= 0 and rva < oracle[i][0] + oracle[i][1]

    refs = reloc_target_refs()
    band_of = band_lookup()
    is_game = game_site_test(model)
    covered = _alias_covered(model)
    fns = sorted((b.rva, b.size, b.unit) for b in model.functions
                 if b.channel in UNIT_CHANNELS and b.unit)
    fn_starts = [f[0] for f in fns]

    def unit_of(site: int) -> str | None:
        i = bisect.bisect_right(fn_starts, site) - 1
        if i >= 0 and site < fns[i][0] + fns[i][1]:
            return fns[i][2]
        return None

    census = censuses.data()
    census_starts = [r["rva"] for r in census]
    out = []
    for rva in fences:
        if oracled(rva):
            continue
        sites = [s for s in refs.get(rva, ())
                 if is_game(s) and not covered(rva, s)]
        if not sites:
            continue
        i = bisect.bisect_right(census_starts, rva) - 1
        row = census[i] if i >= 0 and rva < census[i]["rva"] + census[i]["size"] \
            else None
        units = {u for s in sites if (u := unit_of(s))}
        out.append({"rva": rva, "sites": sites,
                    "bands": sorted({band_of(s) for s in sites}),
                    "units": sorted(units), "census": row})
    return out


def format_unprovisioned(rows) -> list[str]:
    """One worklist line per unprovisioned target."""
    lines = []
    for r in rows:
        c = r["census"]
        if c is None:
            census = "census=?"
        else:
            interior = "" if c["rva"] == r["rva"] \
                else f" interior of 0x{c['rva']:06x}"
            census = f"census kind={c['kind'] or 'datum'} {c['region']}{interior}"
        sites = ",".join(f"0x{s:06x}" for s in r["sites"][:4])
        more = f" +{len(r['sites']) - 4}" if len(r["sites"]) > 4 else ""
        lines.append(f"0x{r['rva']:06x}  {census}  "
                     f"bands={','.join(r['bands'])}  "
                     f"units={','.join(r['units']) or '-'}  "
                     f"sites={sites}{more}")
    return lines


def worklist(model: Model) -> list[dict]:
    """Recompute the data-identity pipeline (no YAML, no build outputs) and
    return the unprovisioned rows."""
    names_map = unit_names(model)
    band = eh_band.groups(retail().pe.path, names_map)
    data_names = {b.rva: b.name for b in model.data if b.channel and b.name}
    for rva, name, _unit, _size in eh_band.data_records(band):
        data_names.setdefault(rva, name)
    rdata_syms, data_syms = data_symbols(model, data_names)
    return unprovisioned_rows(rdata_syms, data_syms, model)


# --- YAML emission -----------------------------------------------------------

def func_source_file(rva: int, names_map) -> str:
    """`c:\\proj\\<unit>.c` for a claimed function, else the address bucket
    `c:\\proj\\seg_NNNN.cpp` (both under the engine root the delinker strips)."""
    if rva in names_map:
        return r"c:\proj\%s.c" % names_map[rva][1]
    text_lo, _hi = sections_of()[".text"]
    bucket = (rva - text_lo) >> BUCKET_SHIFT
    return r"c:\proj\seg_%04x.cpp" % bucket


def _emit_module(funcs, rdata_syms, data_syms, iat_syms, names_map,
                 w, text_base, text_segment, location, module_path):
    func_files, files_seen, files_set = [], [], set()
    for rva, _size, _name in funcs:
        sf = func_source_file(rva, names_map)
        func_files.append(sf)
        if sf not in files_set:
            files_set.add(sf)
            files_seen.append(sf)

    w("    - Module:          '%s'\n" % module_path)
    w("      ObjFile:         '%s'\n" % module_path)

    w("      SourceFiles:\n")
    for sf in files_seen:
        w("        - '%s'\n" % sf)
    w("      Subsections:\n")
    w("        - !FileChecksums\n")
    w("          Checksums:\n")
    for sf in files_seen:
        # Deterministic 16-byte checksum from the path (content is fake).
        cks = hashlib.md5(sf.encode()).hexdigest()
        w("            - FileName:        '%s'\n" % sf)
        w("              Kind:            MD5\n")
        w("              Checksum:        %s\n" % cks.upper())
    for (rva, size, _name), sf in zip(funcs, func_files):
        off = rva - text_base
        w("        - !Lines\n")
        w("          CodeSize:        %d\n" % size)
        w("          Flags:           [  ]\n")
        w("          RelocOffset:     %d\n" % off)
        w("          RelocSegment:    %d\n" % text_segment)
        w("          Blocks:\n")
        w("            - FileName:        '%s'\n" % sf)
        w("              Lines:\n")
        w("                - Offset:          0\n")
        w("                  LineStart:       1\n")
        w("                  EndDelta:        0\n")
        w("                  IsStatement:     true\n")
        w("              Columns:         []\n")

    w("      Modi:\n")
    w("        Records:\n")
    for rva, size, name in funcs:
        off = rva - text_base
        w("          - Kind:            S_GPROC32\n")
        w("            ProcSym:\n")
        w("              CodeSize:        %d\n" % size)
        w("              DbgStart:        0\n")
        w("              DbgEnd:          0\n")
        w("              FunctionType:    0\n")
        w("              Offset:          %d\n" % off)
        w("              Segment:         %d\n" % text_segment)
        w("              Flags:           [  ]\n")
        w("              DisplayName:     '%s'\n" % sanitize_name(name))
        w("          - Kind:            S_END\n")
        w("            ScopeEndSym:     {}\n")
    # Import directories may be merged into .rdata. Segment numbers and
    # offsets always come from the actual PE section table, including IAT.
    seen_data = {}
    for rva, name in rdata_syms + data_syms + iat_syms:
        if rva in seen_data:
            if seen_data[rva] != name:
                raise ValueError(f"conflicting PDB data identities at {rva:#x}")
            continue
        seen_data[rva] = name
        offset, segment = location(rva)
        w("          - Kind:            S_LDATA32\n")
        w("            DataSym:\n")
        w("              Type:            0\n")
        w("              Offset:          %d\n" % offset)
        w("              Segment:         %d\n" % segment)
        w("              DisplayName:     '%s'\n" % sanitize_name(name))

    return files_seen


def emit_yaml(funcs, rdata_syms, data_syms, iat_syms, names_map, out, folded_aliases=()) -> None:
    """Write the yaml2pdb description: C13 line info in owned modules that
    attributes each function to its synthetic source file, S_GPROC32 records
    for .text and S_LDATA32 for .rdata/.data/.idata."""
    pe = retail().pe
    text_sec = pe.section(".text")
    text_base = text_sec["va"]
    text_segment = pe.sections.index(text_sec) + 1

    def location(rva):
        for segment, sec in enumerate(pe.sections, 1):
            if sec["va"] <= rva < sec["va"] + max(sec["vsize"], sec["rsize"]):
                return rva - sec["va"], segment
        raise ValueError(f"PDB symbol at {rva:#x} outside PE sections")

    w = out.write

    w("MSF:\n")
    w("  SuperBlock:\n")
    w("    BlockSize:       4096\n")
    w("    FreeBlockMap:    2\n")
    w("    NumBlocks:       0\n")
    w("    NumDirectoryBytes: 0\n")
    w("    Unknown1:        0\n")
    w("    BlockMapAddr:    0\n")
    w("PdbStream:\n")
    w("  Age:             1\n")
    w("  Guid:            '{00000000-0000-0000-0000-000000000000}'\n")
    w("  Signature:       0\n")
    w("  Features:        [ VC140 ]\n")
    w("  Version:         VC70\n")
    w("DbiStream:\n")
    w("  VerHeader:       V70\n")
    w("  Age:             1\n")
    w("  BuildNumber:     0\n")
    w("  PdbDllVersion:   0\n")
    w("  PdbDllRbld:      0\n")
    w("  Flags:           0\n")
    w("  MachineType:     x86\n")
    w("  Modules:\n")
    files_seen = _emit_module(funcs, rdata_syms, data_syms, iat_syms, names_map,
                              w, text_base, text_segment, location, MODULE_PATH)
    for index, (rva, size, name, unit) in enumerate(folded_aliases):
        alias_files = _emit_module(
            [(rva, size, name)], [], [], [], {rva: (name, unit, size)},
            w, text_base, text_segment, location,
            r"c:\proj\folded_alias_%d.obj" % index)
        for sf in alias_files:
            if sf not in files_seen:
                files_seen.append(sf)

    # Top-level PDB string table: the source paths line info references.
    w("StringTable:\n")
    for sf in files_seen:
        w("  - '%s'\n" % sf)


# --- DBI header patch ---------------------------------------------------------
#
# DBIHeader layout (pdb2 crate / microsoft-pdb): u16 symbol_records_stream at
# offset 0x14. yaml2pdb writes 0xFFFF (nil) there when no GSI is present; we
# repoint it at an existing empty stream so pdb2's global_symbols() returns an
# empty table and the module iteration supplies all the real data.

SYMREC_OFFSET_IN_DBI = 0x14
_MSF_MAGIC = b"Microsoft C/C++ MSF 7.00\r\n\x1aDS\x00\x00\x00"


def find_empty_stream_index(pdb_path: Path) -> int | None:
    """Index of an existing 0-byte stream (e.g. /LinkInfo), via pdbutil dump."""
    import re
    from hobbit.tool import pdbutil
    for line in pdbutil.dump(pdb_path, "--streams").splitlines():
        m = re.search(r"Stream\s+(\d+)\s+\(\s*0 bytes\)", line)
        if m:
            return int(m.group(1))
    return None


def dbi_stream_file_offset(pdb_path: Path) -> int:
    """File offset of the DBI stream (stream 3) first byte, from the MSF
    superblock + stream directory. The DBI header always fits the first
    block (block size 4096)."""
    data = Path(pdb_path).read_bytes()
    if not data.startswith(_MSF_MAGIC):
        raise ValueError(f"{pdb_path}: unexpected MSF magic")
    (block_size, _free_map, _num_blocks, num_dir_bytes,
     _unknown, block_map_addr) = struct.unpack_from("<IIIIII", data, 32)
    num_dir_blocks = (num_dir_bytes + block_size - 1) // block_size
    dir_block_ptrs = struct.unpack_from(
        "<%dI" % num_dir_blocks, data, block_map_addr * block_size)
    directory = b"".join(
        data[b * block_size: b * block_size + block_size]
        for b in dir_block_ptrs)[:num_dir_bytes]
    pos = 0
    (num_streams,) = struct.unpack_from("<I", directory, pos)
    pos += 4
    sizes = list(struct.unpack_from("<%dI" % num_streams, directory, pos))
    pos += 4 * num_streams
    stream_blocks = []
    for sz in sizes:
        if sz == 0xFFFFFFFF:
            sz = 0
        nblk = (sz + block_size - 1) // block_size
        blks = list(struct.unpack_from("<%dI" % nblk, directory, pos))
        pos += 4 * nblk
        stream_blocks.append(blks)
    DBI_STREAM = 3
    if DBI_STREAM >= num_streams or not stream_blocks[DBI_STREAM]:
        raise RuntimeError("DBI stream missing or empty")
    return stream_blocks[DBI_STREAM][0] * block_size


def patch_symbol_records_stream(pdb_path: Path, target_stream_index: int):
    off = dbi_stream_file_offset(pdb_path) + SYMREC_OFFSET_IN_DBI
    with open(pdb_path, "r+b") as f:
        f.seek(off)
        cur = struct.unpack("<H", f.read(2))[0]
        f.seek(off)
        f.write(struct.pack("<H", target_stream_index))
    return off, cur


# --- driver -------------------------------------------------------------------

def synth(model: Model, out_yaml: Path | None = None, out_pdb: Path | None = None,
          base_dir: Path = BASE_DIR, log=None, yaml_only: bool = False) -> dict:
    """The whole synthesis; returns {names_map, band, funcs, ...} for reuse."""
    if model.violations:
        raise ValueError("refusing PDB synthesis from invalid model: " + "; ".join(model.violations))
    import sys
    log = log or (lambda m: print(f"[pdb_synth] {m}", file=sys.stderr))
    out_yaml = Path(out_yaml or PDB_DIR / "hobbit_named.yaml")
    out_pdb = Path(out_pdb or PDB_DIR / "hobbit_named.pdb")
    exe = retail().pe.path

    names_map = unit_names(model)
    local_dtors = static_dtors.provision(model, names_map, base_dir, retail())
    names_map.update(local_dtors)
    initializers = dyninit.provision(model, names_map, base_dir, retail())
    names_map.update(initializers)
    log(f"global initializers: provisioned {len(initializers)} pinned helper(s)")
    log(f"local-static destructors: provisioned {len(local_dtors)} pinned callback(s)")
    band = eh_band.groups(exe, names_map)
    band_spans = [(g.start, g.end) for g in band]
    for rva, name, unit, size in eh_band.records(band):
        names_map.setdefault(rva, (name, unit, size))
    log(f"EH funclet band: {len(band)} group(s), "
        f"{sum(g.end - g.start for g in band)} B, "
        f"over {len({g.unit for g in band})} owning unit(s)")

    # Static-lib labels: rename-only (the unit stays empty, so library code is
    # partitioned into the linker bucket, never a TU). An ILT-band forwarder's
    # label is deferred so the forwarded BODY's name can win the slot.
    library_rvas: set[int] = set()
    deferred_thunks: dict[int, str] = {}
    nlib = 0
    for b in model.functions:
        if b.channel != "functions_static_libs" or b.rva in names_map:
            continue
        if b.kind == "thunk" and b.size == 5:
            deferred_thunks[b.rva] = b.name
            continue
        names_map[b.rva] = (b.name, "", 0)
        library_rvas.add(b.rva)
        nlib += 1
    if nlib:
        log(f"applied {nlib} tracked library symbol name(s)")

    data_names = {b.rva: b.name for b in model.data if b.channel and b.name}
    neh = 0
    for rva, name, _unit, _size in eh_band.data_records(band):
        if data_names.setdefault(rva, name) == name:
            neh += 1
    log(f"EH funcinfo: named {neh} `.xdata$x` datum(s)")

    thunk_names = ilt_thunk_names(model, names_map)
    if thunk_names:
        log(f"propagated {len(thunk_names)} curated body name(s) to ILT thunks")
    nfold = 0
    for rva, name in sorted(deferred_thunks.items()):
        if rva in thunk_names:
            nfold += 1
            continue
        names_map[rva] = (name, "", 0)
        library_rvas.add(rva)
    if nfold:
        log(f"{nfold} carved ILT thunk label(s) superseded by the body name")

    iat_syms, unresolved = implib.resolve_iat(retail().import_slots(), base_dir)
    log(f"IAT: named {len(iat_syms)}/{len(iat_syms) + len(unresolved)} "
        "IAT slot(s)")
    for slot, label in unresolved:
        log(f"IAT SKIP 0x{slot:06x} {label}: no exact __imp_ decoration "
            "(never guessing @N)")
    import_thunks = import_thunk_names(iat_syms, names_map)

    funcs = function_records(model, names_map, thunk_names, import_thunks,
                             band_spans, log)
    folded_aliases = folded_alias_records(model, funcs)
    rdata_syms, data_syms = data_symbols(model, data_names, base_dir, log)
    unprov = unprovisioned_rows(rdata_syms, data_syms, model)
    if unprov:
        log(f"UNPROVISIONED: {len(unprov)} game-referenced data target(s) "
            "lack a provided identity (the delinker refuses to emit these):")
        for line in format_unprovisioned(unprov[:20]):
            log("  " + line)
    log(f"functions: {len(funcs)}  rdata: {len(rdata_syms)}  "
        f"data: {len(data_syms)}  idata: {len(iat_syms)}  "
        f"named: {len(names_map)}")

    out_yaml.parent.mkdir(parents=True, exist_ok=True)
    with open(out_yaml, "w") as out:
        emit_yaml(funcs, rdata_syms, data_syms, iat_syms, names_map, out,
                  folded_aliases)
    log(f"wrote YAML -> {out_yaml}")
    result = {"names_map": names_map, "band": band, "funcs": funcs,
              "iat_syms": iat_syms, "yaml": out_yaml, "pdb": out_pdb,
              "folded_aliases": folded_aliases}
    if yaml_only:
        return result

    from hobbit.tool import pdbutil
    pdbutil.yaml2pdb(out_yaml, out_pdb)
    empty = find_empty_stream_index(out_pdb)
    if empty is None:
        raise RuntimeError("no empty stream found to repoint symbol records at")
    off, cur = patch_symbol_records_stream(out_pdb, empty)
    log(f"patched DBI symbol_records_stream @0x{off:x}: 0x{cur:04x} -> {empty}")
    return result


def main() -> int:
    import argparse
    from hobbit.model import resolve
    ap = argparse.ArgumentParser(
        prog="python3 -m hobbit.delink.pdb_synth", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--out-yaml", type=Path, default=PDB_DIR / "hobbit_named.yaml")
    ap.add_argument("--out-pdb", type=Path, default=PDB_DIR / "hobbit_named.pdb")
    ap.add_argument("--yaml-only", action="store_true",
                    help="emit YAML and stop (skip yaml2pdb + patch)")
    ap.add_argument("--unprovisioned", action="store_true",
                    help="print the derived remaining-UNPROVISIONED worklist "
                         "and exit (status 1 while any remain)")
    a = ap.parse_args()
    if a.unprovisioned:
        rows = worklist(resolve())
        for line in format_unprovisioned(rows):
            print(line)
        print(f"[pdb_synth] {len(rows)} unprovisioned game-referenced "
              "target(s)")
        return 1 if rows else 0
    synth(resolve(), a.out_yaml, a.out_pdb, yaml_only=a.yaml_only)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
