"""hobbit.lineage - the Entropy sibling-source correspondence workflow.

    hobbit lineage discover --source <entropy-tree> [--commit REV]
        Discover every mechanically plausible lineage candidate. The command
        is read-only and prints candidates not covered by the canonical ledger.

    hobbit lineage inventory [--todo] [--module MODULE]
        Render the queue from config/entropy_lineage.tsv. The queue is sorted
        by the declared dependency wave, then ascending historical MAX.

    hobbit lineage verify [--source <entropy-tree>] [--complete]
        Validate the ledger. With --source, also verify source blob identities
        and candidate coverage. --complete additionally refuses pending rows.

config/entropy_lineage.tsv is the ONE source of truth for decisions. Other
documentation may explain the general source-oracle method or cite a row id;
it must not restate why a particular surviving source fact was not adopted.
"""

from __future__ import annotations


def main(argv=None) -> int:
    import argparse

    ap = argparse.ArgumentParser(
        prog="hobbit lineage",
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    sub = ap.add_subparsers(dest="verb", required=True)

    d = sub.add_parser("discover", help="find plausible source-lineage candidates")
    d.add_argument("--source", required=True, help="local Entropy sibling tree")
    d.add_argument("--commit", help="Git revision; default is HEAD, or source-file tree hash without Git")
    d.add_argument("--json", action="store_true", help="emit JSON")
    d.add_argument("--all", action="store_true", help="include candidates already covered")

    i = sub.add_parser("inventory", help="render the derived adoption queue")
    i.add_argument("--todo", action="store_true", help="show pending rows only")
    i.add_argument("--module", action="append", default=[], help="restrict to module")
    i.add_argument("--json", action="store_true", help="emit JSON")
    i.add_argument("--limit", type=int, default=80, help="maximum rows to print")

    v = sub.add_parser("verify", help="validate ledger and optional source coverage")
    v.add_argument("--source", help="checkout used to verify blobs and discovery coverage")
    v.add_argument("--commit", help="Git revision or a source-file tree hash")
    v.add_argument("--complete", action="store_true", help="also reject pending decisions")

    args = ap.parse_args(argv)
    if args.verb == "discover":
        from hobbit.lineage.discovery import main as discover_main

        return discover_main(args)
    if args.verb == "inventory":
        from hobbit.lineage.ledger import inventory_main

        return inventory_main(args)
    from hobbit.lineage.ledger import verify_main

    return verify_main(args)
