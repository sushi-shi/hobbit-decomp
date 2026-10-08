"""hobbit.sema.image - the retail image, scanned for address queries.

Three whole-image scans sit on top of hobbit.core.pe, shared by every sema
view and computed once per process:

  * PE HIGHLOW or reviewed /FIXED address fields paired with
    the address each stores. Hobbit retains no PE relocation directory:
    the reviewed instruction/typed-data manifest supplies admitted fields
    while preserving its evidence and executable hash. No pointer-sized
    integer scan is promoted to relocation evidence;
  * the admitted decoded E8/E9 rel32 call index (target -> call/jmp sites);
  * the printable-string table outside .text.

Nothing here knows what a label MEANS - that is hobbit.sema.index's job.
"""

from __future__ import annotations

import bisect
import re
import struct
from functools import lru_cache

from hobbit.core.pe import Pe, image as _image


class Image:
    """The retail image plus the scans. `Image()` uses the process-wide Pe."""

    def __init__(self, pe: Pe | None = None):
        self.pe = pe or _image()
        self.base = self.pe.image_base
        t = self.pe.section(".text")
        self.text_lo = t["va"]
        self.text_hi = t["va"] + t["vsize"]
        self._reloc: dict[int, int] | None = None
        self._referents: dict[int, list[int]] | None = None
        self._calls: dict[int, list[tuple[int, int]]] | None = None
        self._strings: dict[int, str] | None = None
        self._str_starts: list[int] | None = None
        self._sites: list[int] | None = None
        self._targets: list[int] | None = None

    # --- raw access ---------------------------------------------------------

    def section_of(self, rva: int) -> dict | None:
        for s in self.pe.sections:
            if s["va"] <= rva < s["va"] + max(s["vsize"], s["rsize"]):
                return s
        return None

    def section_name(self, rva: int) -> str:
        s = self.section_of(rva)
        return s["name"] if s else "?"

    def read(self, rva: int, size: int) -> bytes | None:
        return self.pe.read(rva, size)

    def u32(self, rva: int) -> int | None:
        b = self.pe.read(rva, 4)
        return struct.unpack_from("<I", b)[0] if b else None

    def is_text(self, rva: int) -> bool:
        return self.text_lo <= rva < self.text_hi

    # --- base relocations ---------------------------------------------------

    @property
    def reloc(self) -> dict[int, int]:
        """{fixup_site_rva: target_rva} for PE HIGHLOW or hash-bound reviewed /FIXED fields.

        The site is where the 4-byte address is STORED; the target is the rva
        that address points at (stored VA minus the image base)."""
        if self._reloc is None:
            from hobbit.delink.image import Image as AddressImage
            out: dict[int, int] = {}
            for site in AddressImage(self.pe).absolute_sites:
                val = self.u32(site)
                if val is not None:
                    out[site] = val - self.base
            self._reloc = out
        return self._reloc

    @property
    def referents(self) -> dict[int, list[int]]:
        """{target_rva: [site_rva, ...]} - the reverse of `reloc`, sorted."""
        if self._referents is None:
            out: dict[int, list[int]] = {}
            for site, tgt in self.reloc.items():
                out.setdefault(tgt, []).append(site)
            for sites in out.values():
                sites.sort()
            self._referents = out
        return self._referents

    def refs_to_range(self, lo: int, hi: int) -> list[tuple[int, int]]:
        """[(site, target)] for every stored address landing in [lo, hi) -
        interior references included (a `clip+0x64` operand references clip)."""
        if self._targets is None:
            self._targets = sorted(self.referents)
        out = []
        i = bisect.bisect_left(self._targets, lo)
        while i < len(self._targets) and self._targets[i] < hi:
            tgt = self._targets[i]
            out += [(site, tgt) for site in self.referents[tgt]]
            i += 1
        return sorted(out)

    def relocs_in(self, lo: int, hi: int) -> list[tuple[int, int]]:
        """[(site, target)] for every fixup site inside [lo, hi) - the address
        operands of a function's own bytes."""
        if self._sites is None:
            self._sites = sorted(self.reloc)
        i = bisect.bisect_left(self._sites, lo)
        out = []
        while i < len(self._sites) and self._sites[i] < hi:
            out.append((self._sites[i], self.reloc[self._sites[i]]))
            i += 1
        return out

    # --- .text call graph ---------------------------------------------------

    @property
    def call_index(self) -> dict[int, list[tuple[int, int]]]:
        """{target_rva: [(site_rva, opcode)]} for admitted decoded E8/E9 rel32 instructions in .text; the
        hash-bound census proves boundaries and bytes revalidate each target."""
        if self._calls is None:
            import hashlib
            from hobbit.core.paths import RETAIL
            from hobbit.core.tsv import read
            path = RETAIL / "pc/direct_branches.tsv"
            banner, header, rows = read(path)
            if "# image-sha256: " + hashlib.sha256(self.pe.data).hexdigest() not in banner:
                raise ValueError(f"{path}: direct branch evidence does not identify this image")
            idx: dict[int, list[tuple[int, int]]] = {}
            for row in rows:
                site, target = int(row["site_rva"], 0), int(row["target_rva"], 0)
                op = int(row["opcode"], 0)
                code = self.read(site, 5)
                if (op not in (0xe8, 0xe9) or code is None or code[0] != op
                        or site + 5 + struct.unpack_from("<i", code, 1)[0] != target
                        or not self.is_text(site) or not self.is_text(target)):
                    raise ValueError(f"{path}: invalid decoded direct branch at {site:#x}")
                idx.setdefault(target, []).append((site, op))
            self._calls = idx
        return self._calls

    def jmp_target(self, rva: int) -> int | None:
        """Body a bare `E9 rel32` forwarder at `rva` jumps to, else None."""
        b = self.pe.read(rva, 5)
        if not b or b[0] != 0xE9:
            return None
        tgt = rva + 5 + struct.unpack_from("<i", b, 1)[0]
        return tgt if self.is_text(tgt) else None

    def thunks_to(self, target: int) -> list[int]:
        """Sites of `jmp target` forwarders - what vtables and command tables
        store instead of the body address."""
        return [site for site, op in self.call_index.get(target, ())
                if op == 0xE9 and self.jmp_target(site) == target]

    # --- strings ------------------------------------------------------------

    @property
    def strings(self) -> dict[int, str]:
        """{start_rva: text} for every printable run (>=4) outside .text."""
        if self._strings is None:
            pat = re.compile(rb"[\x20-\x7e]{4,}")
            out: dict[int, str] = {}
            for s in self.pe.sections:
                if s["name"] == ".text":
                    continue
                blob = self.pe.data[s["rptr"]:s["rptr"] + s["rsize"]]
                for m in pat.finditer(blob):
                    out[s["va"] + m.start()] = m.group().decode("latin-1")
            self._strings = out
        return self._strings

    def string_at(self, rva: int) -> tuple[int, str] | None:
        """(start, text) of the printable run covering `rva`, or None."""
        if self._str_starts is None:
            self._str_starts = sorted(self.strings)
        i = bisect.bisect_right(self._str_starts, rva) - 1
        if i < 0:
            return None
        start = self._str_starts[i]
        text = self.strings[start]
        return (start, text) if rva < start + len(text) + 1 else None


@lru_cache(maxsize=1)
def retail() -> Image:
    """The process-wide retail Image."""
    return Image()
