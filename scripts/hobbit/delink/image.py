"""hobbit.delink.image - retail-image facts core/pe does not carry.

The delink oracles need the PE's base-relocation directory (the exact DIR32
address-operand sites), raw-offset reads, C strings, import-directory slots
and the initialized-vs-loader-zero storage classification with its
FileAlignment ambiguity band. Ported from the old tree's core/pe.py +
core/data_audit.py; layered here because only delink consumes them.
"""

from __future__ import annotations

import bisect
import csv
import hashlib
import struct
from functools import lru_cache
from pathlib import Path

from hobbit.core.pe import Pe, image


class Image:
    """One retail image + the delink-side derived views, cached."""

    def __init__(self, pe: Pe | None = None):
        self.pe = pe or image()
        self.data = self.pe.data
        self.image_base = self.pe.image_base
        d = self.data
        pe_off = struct.unpack_from("<I", d, 0x3C)[0]
        self._opt = pe_off + 24
        self.file_alignment = struct.unpack_from("<I", d, self._opt + 36)[0]
        self._reloc_sites: list[int] | None = None
        self._absolute_sites: list[int] | None = None

    # --- addressing ---------------------------------------------------------
    def off(self, rva: int) -> int | None:
        """File offset of `rva`, or None if unmapped OR past the section's raw
        data (a virtual-only tail has no file bytes)."""
        for s in self.pe.sections:
            if s["va"] <= rva < s["va"] + max(s["vsize"], s["rsize"]):
                o = rva - s["va"] + s["rptr"]
                return o if o < s["rptr"] + s["rsize"] else None
        return None

    def sec_name(self, rva: int) -> str | None:
        for s in self.pe.sections:
            if s["va"] <= rva < s["va"] + max(s["vsize"], s["rsize"]):
                return s["name"]
        return None

    def u32(self, rva: int) -> int | None:
        raw = self.raw_read(rva, 4)
        return struct.unpack("<I", raw)[0] if raw is not None else None

    def cstr(self, rva: int, n: int = 512) -> str | None:
        value = self.cstring(rva, n)
        return value.decode("latin1") if value is not None else None

    def cstring(self, rva: int, limit: int = 512) -> bytes | None:
        """NUL-terminated raw bytes at `rva`, bounded to the section's RAW
        extent (the string-pool oracle's read - matches the old Exe.cstring)."""
        for s in self.pe.sections:
            if s["va"] <= rva < s["va"] + max(s["vsize"], s["rsize"]):
                off = s["rptr"] + (rva - s["va"])
                cap = min(s["rptr"] + s["rsize"], off + limit)
                end = off
                while end < cap and self.data[end] != 0:
                    end += 1
                return self.data[off:end] if end < cap else None
        return None

    def payload(self, rva: int, n: int) -> bytes:
        """Retail's bytes at [rva, rva+n); a virtual-only tail reads as zero."""
        value = self.pe.read(rva, n)
        if value is None:
            raise ValueError(f"payload [{rva:#x}, {rva + n:#x}) is outside mapped storage")
        return value

    # --- base relocations (the real DIR32 address-operand sites) ------------
    @property
    def reloc_sites(self) -> list[int]:
        """Sorted RVAs of every IMAGE_REL_BASED_HIGHLOW site (data dir 5)."""
        if self._reloc_sites is None:
            rva, size = self.directory(5)
            sites = []
            if rva or size:
                if not rva or not size:
                    raise ValueError("incomplete PE base relocation directory")
                blob = self.raw_read(rva, size)
                if blob is None:
                    raise ValueError("base relocation directory is not raw-backed")
                pos = 0
                while pos < len(blob):
                    if pos + 8 > len(blob):
                        raise ValueError("truncated base relocation header")
                    page, block = struct.unpack_from("<II", blob, pos)
                    if block == 0:
                        if any(blob[pos:]):
                            raise ValueError("nonzero trailing relocation bytes")
                        break
                    if block < 8 or block % 2 or pos + block > len(blob):
                        raise ValueError("invalid base relocation block")
                    for offset in range(pos + 8, pos + block, 2):
                        entry, = struct.unpack_from("<H", blob, offset)
                        if entry >> 12 == 3:
                            site = page + (entry & 0xfff)
                            if self.raw_read(site, 4) is None:
                                raise ValueError(f"invalid HIGHLOW site {site:#x}")
                            sites.append(site)
                    pos += block
            self._reloc_sites = sorted(set(sites))
        return self._reloc_sites

    def directory(self, index: int) -> tuple[int, int]:
        count, = struct.unpack_from("<I", self.data, self._opt + 92)
        if index >= count:
            return 0, 0
        return struct.unpack_from("<II", self.data, self._opt + 96 + index * 8)

    def raw_read(self, rva: int, size: int) -> bytes | None:
        """Only file-backed bytes; never permit a read into another section."""
        for sec in self.pe.sections:
            if sec["va"] <= rva and rva + size <= sec["va"] + sec["rsize"]:
                off = sec["rptr"] + rva - sec["va"]
                return self.data[off:off + size]
        return None

    def reviewed_sites(self, manifest: Path | None = None) -> list[int]:
        """Hash-bound /FIXED address fields, adapted from HoMM1 Buka
        d3c9297135800ee0d198b292f2424464d20b6f5c core/pe.py.

        These are reviewed address fields, NOT recovered PE relocation entries.
        The retained PE directory takes precedence. No integer scanning occurs.
        """
        if self.directory(5) != (0, 0):
            return self.reloc_sites
        if manifest is None:
            from hobbit.core.paths import RETAIL
            manifest = RETAIL / "absolute_relocations.tsv"
        lines = Path(manifest).read_text(encoding="utf-8").splitlines()
        expected = "# image-sha256: " + hashlib.sha256(self.data).hexdigest()
        if expected not in lines:
            raise ValueError(f"{manifest}: address manifest is not pinned to {self.pe.path}")
        reader = csv.DictReader((line for line in lines if line and not line.startswith("#")),
                                delimiter="\t")
        if reader.fieldnames != ["site_rva", "kind"]:
            raise ValueError(f"{manifest}: expected site_rva,kind header")
        sites = set()
        for row in reader:
            site = int(row["site_rva"], 0)
            raw = self.raw_read(site, 4)
            if row["kind"] != "dir32" or raw is None or None in row:
                raise ValueError(f"{manifest}: invalid absolute field {site:#x}")
            target = struct.unpack("<I", raw)[0] - self.image_base
            if self.sec_name(target) is None:
                raise ValueError(f"{manifest}: absolute field {site:#x} targets unmapped RVA {target:#x}")
            if site in sites:
                raise ValueError(f"{manifest}: duplicate absolute field {site:#x}")
            sites.add(site)
        return sorted(sites)

    @property
    def absolute_sites(self) -> list[int]:
        """PE HIGHLOW sites or reviewed /FIXED fields, never guessed pointers."""
        if self._absolute_sites is None:
            self._absolute_sites = self.reviewed_sites()
        return self._absolute_sites

    def relocs_in(self, lo: int, hi: int) -> list[int]:
        """Proven absolute fields inside [lo, hi)."""
        sites = self.absolute_sites
        i, j = bisect.bisect_left(sites, lo), bisect.bisect_left(sites, hi)
        return sites[i:j]

    # --- imports -------------------------------------------------------------
    def import_slots(self) -> list[tuple[int, str | None, str, int | None]]:
        """[(iat_slot_rva, name_or_None, dll, ordinal_or_None)] from the PE."""
        d = self.data
        imp_rva = struct.unpack_from("<I", d, self._opt + 96 + 1 * 8)[0]
        if not imp_rva:
            return []

        def raw(rva: int) -> int:
            o = self.off(rva)
            if o is None:
                raise ValueError(f"RVA 0x{rva:x} outside PE raw sections")
            return o

        def cstr(off: int) -> str:
            return d[off:d.index(0, off)].decode("ascii", "replace")

        out = []
        p = raw(imp_rva)
        while True:
            lookup, ts, fw, name_rva, addr_rva = struct.unpack_from("<IIIII", d, p)
            if not any((lookup, ts, fw, name_rva, addr_rva)):
                break
            dll = cstr(raw(name_rva))
            thunk = raw(lookup or addr_rva)
            i = 0
            while True:
                v = struct.unpack_from("<I", d, thunk + i * 4)[0]
                if not v:
                    break
                slot = addr_rva + i * 4
                if v & 0x80000000:
                    out.append((slot, None, dll, v & 0xFFFF))
                else:
                    out.append((slot, cstr(raw(v) + 2), dll, None))
                i += 1
            p += 20
        return out

    # --- storage classification ----------------------------------------------
    def _emitted_content_floor(self, sec: dict) -> int:
        """Largest section offset PROVABLY inside emitted initialized content.

        `SizeOfRawData` is `round_up(E, FileAlignment)` for the true end E of
        the linker's initialized content, so E > raw_size - FileAlignment.
        """
        return max(0, sec["rsize"] - self.file_alignment)

    def _zero_to_raw_edge(self, sec: dict, offset: int) -> bool:
        """Is [offset, raw_size) all zero? Only then can it be padding."""
        start = sec["rptr"] + offset
        end = sec["rptr"] + sec["rsize"]
        return not any(self.data[start:end])

    def classify_storage(self, rva: int) -> str:
        """'rdata' | 'data-initialized' | 'data-unprovable-tail' |
        'data-loader-zero-tail' | 'other-section' | 'outside-image'.

        The unprovable tail is the <FileAlignment all-zero run at .data's raw
        edge, where alignment padding and a zero-valued global are
        byte-identical (fail-closed: callers must not enrol it bare)."""
        rd = self.pe.section(".rdata")
        if rd["va"] <= rva < rd["va"] + rd["vsize"]:
            return "rdata"
        da = self.pe.section(".data")
        if da["va"] <= rva < da["va"] + da["vsize"]:
            offset = rva - da["va"]
            if offset >= da["rsize"]:
                return "data-loader-zero-tail"
            if offset >= self._emitted_content_floor(da) \
                    and self._zero_to_raw_edge(da, offset):
                return "data-unprovable-tail"
            return "data-initialized"
        for s in self.pe.sections:
            if s["va"] <= rva < s["va"] + max(s["vsize"], s["rsize"]):
                return "other-section"
        return "outside-image"


@lru_cache(maxsize=1)
def retail(path: str | None = None) -> Image:
    return Image(Pe(path) if path else image())


def sections_of(path: Path | str | None = None) -> dict[str, tuple[int, int]]:
    """{name: (base, end)} VIRTUAL bounds for .text/.rdata/.data/.idata."""
    pe = Pe(path) if path else image()
    out = {}
    for name in (".text", ".rdata", ".data", ".idata"):
        try:
            s = pe.section(name)
        except KeyError:
            out[name] = (0, 0)
            continue
        out[name] = (s["va"], s["va"] + s["vsize"])
    return out
