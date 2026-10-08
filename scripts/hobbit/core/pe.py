"""hobbit.core.pe - the retail image, parsed once.

Adapted from Gruntz 7d4bd55b99e32f084834d991badf7609889481f4.
Header checks follow HoMM1 core/image.py at
8d3ae6c96fc9b06d9c709fbdbfa181d78997b787. Hobbit clean builds need no .idata.

The PE section table is the authority for every address-space edge; nothing
in the tree hardcodes an image constant. Parsed lazily and cached per
process (the image never changes).
"""

from __future__ import annotations

import struct
from functools import lru_cache
from pathlib import Path

from hobbit.core.paths import retail_exe


class Pe:
    def __init__(self, path: Path | str | None = None):
        self.path = Path(path or retail_exe())
        if path is None:
            from hobbit.core.inputs import read_verified, targets
            self.data = d = read_verified(targets()["game"], self.path)
        else:
            self.data = d = self.path.read_bytes()
        if len(d) < 64 or d[:2] != b"MZ":
            raise ValueError(f"{self.path}: not an MZ executable")
        pe = struct.unpack_from("<I", d, 0x3C)[0]
        if pe + 24 > len(d) or d[pe:pe + 4] != b"PE\0\0":
            raise ValueError(f"{self.path}: not a PE executable")
        machine = struct.unpack_from("<H", d, pe + 4)[0]
        nsec = struct.unpack_from("<H", d, pe + 6)[0]
        optsz = struct.unpack_from("<H", d, pe + 20)[0]
        if optsz < 96 or pe + 24 + optsz + nsec * 40 > len(d):
            raise ValueError(f"{self.path}: truncated PE headers")
        magic = struct.unpack_from("<H", d, pe + 24)[0]
        if machine != 0x14C or magic != 0x10B:
            raise ValueError(f"{self.path}: expected an i386 PE32 image")
        self.image_base = struct.unpack_from("<I", d, pe + 24 + 28)[0]
        self.sections: list[dict] = []
        for i in range(nsec):
            base = pe + 24 + optsz + i * 40
            name = d[base:base + 8].rstrip(b"\0").decode("latin-1")
            vsize, va, rsize, rptr = struct.unpack_from("<IIII", d, base + 8)
            if rsize and rptr + rsize > len(d):
                raise ValueError(f"{self.path}: truncated section {name}")
            self.sections.append({"name": name, "va": va, "vsize": vsize,
                                  "rsize": rsize, "rptr": rptr})

    def section(self, name: str) -> dict:
        s = next((s for s in self.sections if s["name"] == name), None)
        if s is None:
            raise KeyError(f"{self.path}: no section {name}")
        return s

    def text_span(self) -> tuple[int, int]:
        """[lo, hi) of .text's VIRTUAL extent (what function extents cap at)."""
        t = self.section(".text")
        return t["va"], t["va"] + t["vsize"]

    def data_regions(self) -> dict[str, tuple[int, int]]:
        """Initialized data, its zero-fill tail, and optional import storage.

        The clean Hobbit image merges imports into .rdata. A separate .idata
        is included only when the inspected image actually provides it.

        Preserve Gruntz's raw-size regions: file-alignment padding in .rdata
        and initialized .data remains addressable census space. A region is
        an admission bound, not evidence that padding is an independent
        datum. BSS contains only the virtual tail beyond those raw bytes.
        """
        rd, da = self.section(".rdata"), self.section(".data")
        regions = {
            "rdata": (rd["va"], rd["va"] + rd["rsize"]),
            "data": (da["va"], da["va"] + da["rsize"]),
            "bss": (da["va"] + da["rsize"],
                    da["va"] + max(da["vsize"], da["rsize"])),
        }
        it = next((s for s in self.sections if s["name"] == ".idata"), None)
        if it is not None:
            regions["idata"] = (it["va"], it["va"] + max(it["vsize"], it["rsize"]))
        return regions

    def read(self, rva: int, size: int) -> bytes | None:
        """Bytes at rva; loader zero-fill (past a section's raw size) reads as
        ZEROS, never as the next section's file bytes; short reads are None."""
        if size < 0:
            raise ValueError("negative read size")
        for s in self.sections:
            if s["va"] <= rva and rva + size <= s["va"] + max(s["vsize"], s["rsize"]):
                raw_end = s["va"] + s["rsize"]
                if rva >= raw_end:
                    return bytes(size)
                stored = min(size, raw_end - rva)
                off = s["rptr"] + rva - s["va"]
                chunk = self.data[off:off + stored]
                if len(chunk) != stored:
                    return None
                return chunk + bytes(size - stored)
        return None


@lru_cache(maxsize=1)
def image() -> Pe:
    return Pe()
