"""hobbit.sema - the semantic-navigation surface over the retail image.

    hobbit sema rva     <addr>            address dossier: the winning binding,
                                          its aliases, channel, extent, match%
    hobbit sema disasm  <rva|name>        annotated retail i386 assembly
    hobbit sema dump    <rva|name>        raw bytes + relocation targets + asm
    hobbit sema xref    <rva|name>        callers, callees and referent sites
    hobbit sema strings [<rva>|--find s]  string literals a function reaches
    hobbit sema vtable  <rva>             a vtable's slots / who holds a fn
    hobbit sema class   <Class|fn>        a class's vtables, slot by slot
    hobbit sema gaps    [options]         unclaimed same-file .text gaps
    hobbit sema map     [sub ...]         retail address-space map
    hobbit sema match   <unit|rva|name>   objdiff scores for a unit / function

Every module is also a direct entry: `python3 -m hobbit.sema.xref 0x136180`.
`hobbit sema -` is batch mode: newline-delimited view commands on stdin,
answered against ONE loaded Model and image (a 40-query investigation pays one
parse instead of forty).

sema is a READ-ONLY consumer with four inputs and no policy of its own:
the Model (`hobbit.model.resolve`) for identity, the retail image
(`hobbit.core.pe`) for bytes, the compare slice's current report
(`build/objdiff/compare-new/report.json`, falling back to the older
`build/objdiff/report.json`) for scores and `config/units.toml` for the unit
list. It writes nothing.

Doctrine: assembly only. Nothing here decompiles - views annotate real
instruction bytes with Model labels, and a question the labels cannot answer
is reported as unanswered rather than guessed.

rc convention: 0 answered, 1 answered-NO (valid query, no hit / differs),
2 error (bad input, missing prerequisite).
"""

from __future__ import annotations

import sys

SUBCOMMANDS = {
    "rva": "hobbit.sema.rva",
    "disasm": "hobbit.sema.disasm",
    "dump": "hobbit.sema.dump_target",
    "dump_target": "hobbit.sema.dump_target",
    "xref": "hobbit.sema.xref",
    "strings": "hobbit.sema.strings",
    "vtable": "hobbit.sema.vtable",
    "class": "hobbit.sema.classof",
    "classof": "hobbit.sema.classof",
    "gaps": "hobbit.sema.gaps",
    "map": "hobbit.sema.map",
    "match": "hobbit.sema.match",
}


class SemaError(Exception):
    """A real error (bad input, missing prerequisite) - rc 2."""


def die(msg: str):
    """Raise the rc-2 error. Answered-NO paths return 1 instead."""
    raise SemaError(msg)


def parse_rva(text: str) -> int:
    """A hex address, with or without the 0x."""
    try:
        return int(text, 16)
    except ValueError:
        die(f"'{text}' is not a hex RVA (e.g. 0x00153810)")


def resolve_target(token: str) -> list[int]:
    """Candidate rvas for a hex address OR a name spelling. A token that is
    neither is one error, not two: reporting only 'not a hex RVA' for an
    obvious name hides the question the caller actually asked."""
    from hobbit.sema.index import index
    hits = index().resolve_name(token)
    if not hits:
        die(f"'{token}' is not a hex RVA and no binding carries that name")
    return hits


def _drop_stdout() -> None:
    """Point stdout at /dev/null so the interpreter's exit-time flush cannot
    raise a second BrokenPipeError (which python reports as rc 120 plus an
    'Exception ignored' line - `hobbit sema ... | head` used to end that way)."""
    import contextlib
    import os
    with contextlib.suppress(OSError, ValueError):
        os.dup2(os.open(os.devnull, os.O_WRONLY), sys.stdout.fileno())


def run(module: str, argv: list[str]) -> int:
    """Run one subcommand module's main(), mapping SemaError to rc 2."""
    import importlib
    try:
        rc = importlib.import_module(module).main(argv)
        sys.stdout.flush()          # surface a closed pipe HERE, not at exit
        return rc
    except SemaError as e:
        print(f"[sema] ERROR: {e}", file=sys.stderr)
        return 2
    except BrokenPipeError:
        _drop_stdout()              # the reader went away: a normal end
        return 0


def batch() -> int:
    """Answer newline-delimited view commands from stdin in one process."""
    import shlex
    rc = 0
    try:
        for line in sys.stdin:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            print(f"== hobbit sema {line}")
            rc = main(shlex.split(line)) or rc
            sys.stdout.flush()
    except BrokenPipeError:
        _drop_stdout()
        return 0
    return rc


def main(argv: list[str] | None = None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)
    if not argv or argv[0] in ("-h", "--help", "help"):
        print(__doc__.strip())
        return 0 if argv else 2
    if argv[0] == "-":
        return batch()
    sub, rest = argv[0], argv[1:]
    if sub not in SUBCOMMANDS:
        print(f"hobbit sema: unknown view {sub!r} (have: "
              f"{', '.join(sorted(set(SUBCOMMANDS)))})", file=sys.stderr)
        return 2
    return run(SUBCOMMANDS[sub], rest)
