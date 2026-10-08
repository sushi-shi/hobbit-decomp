"""hobbit.rsrc.res - the two resource containers, read.

`read_pe_rsrc` walks a PE's .rsrc directory and returns the resources in
PAYLOAD-ADDRESS order - the order the original .rc's statements appeared
(rc.exe emits, and cvtres/link keep, statement order; the directory tree is
sorted separately by the linker, so payload order is recovered source
structure). `read_res` parses the Win32 .RES container rc.exe writes, in
container order. Both yield (type, name, lang, payload) rows, directly
comparable; .RES memory flags are dropped (the PE directory has no field for
them - they cannot change a linked byte).
"""

from __future__ import annotations

import struct
from pathlib import Path

from hobbit.core.pe import Pe

#: RT_* ordinals -> names (winuser.h; 240/241 are MFC's afxres.h privates).
RT_NAME = {
    1: "CURSOR", 2: "BITMAP", 3: "ICON", 4: "MENU", 5: "DIALOG", 6: "STRING",
    7: "FONTDIR", 8: "FONT", 9: "ACCELERATOR", 10: "RCDATA",
    11: "MESSAGETABLE", 12: "GROUP_CURSOR", 14: "GROUP_ICON", 16: "VERSION",
    17: "DLGINCLUDE", 19: "PLUGPLAY", 20: "VXD", 21: "ANICURSOR",
    22: "ANIICON", 23: "HTML", 24: "MANIFEST", 240: "DLGINIT", 241: "TOOLBAR",
}


def rt(rtype: int | str) -> str:
    return RT_NAME.get(rtype, str(rtype)) if isinstance(rtype, int) else rtype


def read_pe_rsrc(pe: Pe) -> list[tuple[int | str, int | str, int, int, bytes]]:
    """The PE's .rsrc as [(type, name, lang, codepage, payload)] rows in
    payload-address order."""
    rs = pe.section(".rsrc")
    base, off0, d = rs["va"], rs["rptr"], pe.data
    out: list[tuple] = []

    def walk(dir_rva: int, ids: list) -> None:
        o = off0 + (dir_rva - base)
        nnamed, nid = struct.unpack_from("<HH", d, o + 12)
        for i in range(nnamed + nid):
            nameoff, dataoff = struct.unpack_from("<II", d, o + 16 + i * 8)
            if nameoff & 0x80000000:
                so = off0 + (nameoff & 0x7FFFFFFF)
                ln = struct.unpack_from("<H", d, so)[0]
                ident: int | str = d[so + 2:so + 2 + ln * 2].decode("utf-16-le")
            else:
                ident = nameoff
            if dataoff & 0x80000000:
                walk(base + (dataoff & 0x7FFFFFFF), ids + [ident])
            else:
                drva, dsz, cp, _ = struct.unpack_from("<IIII", d, off0 + dataoff)
                po = off0 + (drva - base)
                t, nm, lg = (ids + [ident])[:3]
                out.append((t, nm, lg, cp, d[po:po + dsz], drva))

    walk(base, [])
    out.sort(key=lambda r: r[5])
    return [r[:5] for r in out]


def _res_ident(d: bytes, p: int) -> tuple[int | str, int]:
    """A .RES type/name field: 0xFFFF <WORD> = ordinal, else UTF-16 sz."""
    if struct.unpack_from("<H", d, p)[0] == 0xFFFF:
        return struct.unpack_from("<H", d, p + 2)[0], p + 4
    e = p
    while struct.unpack_from("<H", d, e)[0]:
        e += 2
    return d[p:e].decode("utf-16-le"), e + 2


def read_res(path: Path | str) -> list[tuple[int | str, int | str, int, bytes]]:
    """A Win32 .RES container as [(type, name, lang, payload)] rows in
    container order; the leading 32-byte null entry is skipped."""
    d = Path(path).read_bytes()
    out, p = [], 0
    while p + 8 <= len(d):
        dsz, hsz = struct.unpack_from("<II", d, p)
        rtype, q = _res_ident(d, p + 8)
        name, q = _res_ident(d, q)
        q += (-(q - p)) % 4
        _dv, _mf, lang = struct.unpack_from("<IHH", d, q)
        if not (rtype == 0 and name == 0):
            out.append((rtype, name, lang, d[p + hsz:p + hsz + dsz]))
        p += hsz + dsz
        p += (-p) % 4
    return out
