"""hobbit.rsrc - the resource section, proven from source.

    hobbit rsrc check               compile src/Apps/Meridian/Meridian.rc with the era
                                    RC.EXE and byte-compare every payload
                                    (type, name, lang, bytes, payload order)
                                    against the retail PE's .rsrc - total
                                    coverage both directions. Exit 0 identical,
                                    1 a real deviation, 2 could not run (no era
                                    rc.exe / unwritable --out / unreadable PE)

src/Apps/Meridian/Meridian.rc describes the pinned PC image's ten resources.
The nine icon payloads are extracted locally from the verified user-supplied
executable into ignored build/gen/rsrc/. The era RC.EXE compiles them; the
resource gate checks the result against the reference PE.
"""

from __future__ import annotations

_SUBS = ("check",)


def main(argv=None) -> int:
    import sys
    argv = list(sys.argv[1:] if argv is None else argv)
    if argv and argv[0] in ("-h", "--help"):
        print(__doc__.strip())
        return 0
    if not argv or argv[0] not in _SUBS:
        print(__doc__.strip(), file=sys.stderr)
        what = f"unknown subcommand {argv[0]!r}" if argv else "no subcommand"
        print(f"\nhobbit rsrc: {what} - pick one of: {', '.join(_SUBS)}",
              file=sys.stderr)
        return 2
    sub, rest = argv[0], argv[1:]
    from hobbit.rsrc.check import main as check_main
    sys.argv = [f"hobbit rsrc {sub}", *rest]
    return check_main(rest)
