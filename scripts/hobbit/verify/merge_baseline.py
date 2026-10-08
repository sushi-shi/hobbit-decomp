"""hobbit.verify.merge_baseline - three-way row merge of config/match_baseline.tsv.

    hobbit verify merge-baseline                   # during a rebase conflict (:1: base, :2: main, :3: lane)
    hobbit verify merge-baseline BASE MAIN LANE    # three revisions

Per function: if only one side changed the row since the fork point, take that
side; if both changed it, take the higher MAX (the lane's row on a tie) with
HIST = max. A row "changed" when its MAX, fingerprint, or HIST moved; CUR moves
on every bank and is ignored. Taking one side wholesale either drops a lane's
banked MAX (a probe-banked MAX cannot be re-banked) or revives a stale
fingerprint that makes the next bank reset a MAX.
"""

from __future__ import annotations

import subprocess
import sys

PATH = "config/match_baseline.tsv"


def _rows(text: str) -> tuple[list[str], dict]:
    head, body, in_functions = [], {}, False
    for line in text.splitlines():
        if line.startswith("# [functions]\t"):
            in_functions = True
        if not in_functions or line.startswith("#"):
            head.append(line)
            continue
        if line:
            cols = line.split("\t")
            body[(cols[0], cols[1])] = cols
    return head, body


def _earned(row):
    """The fields a bank earns: MAX, fingerprint, HIST."""
    return None if row is None else (row[2], row[5], row[7] if len(row) > 7 else "")


def merge(base_text: str, main_text: str, lane_text: str) -> tuple[str, int]:
    """(merged file text, number of rows taken from the lane)."""
    _, base = _rows(base_text)
    head, main = _rows(main_text)
    _, lane = _rows(lane_text)
    out, taken = {}, 0
    for key in main.keys() | lane.keys():
        m, l, b = main.get(key), lane.get(key), base.get(key)
        if l is None:
            out[key] = m
        elif m is None or _earned(m) == _earned(b):
            out[key] = l
            taken += _earned(l) != _earned(b)
        elif _earned(l) == _earned(b):
            out[key] = m
        else:
            row = list(l if float(l[2]) >= float(m[2]) else m)
            hist = [float(x[7]) for x in (m, l) if len(x) > 7 and x[7]]
            if hist and len(row) > 7:
                row[7] = f"{max(hist):.4f}"
            taken += row[5] == l[5]
            out[key] = row
    lines = head + ["\t".join(out[k]) for k in sorted(out)]
    return "\n".join(lines) + "\n", taken


def _show(spec: str) -> str:
    return subprocess.run(["git", "show", spec], capture_output=True, text=True,
                          check=True).stdout


def main(argv=None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)
    if len(argv) == 3:
        texts = [_show(f"{rev}:{PATH}") for rev in argv]
    elif not argv:
        texts = [_show(f":{stage}:{PATH}") for stage in (1, 2, 3)]
    else:
        print(__doc__.strip(), file=sys.stderr)
        return 2
    text, taken = merge(*texts)
    with open(PATH, "w") as f:
        f.write(text)
    print(f"merged {PATH}: {taken} row(s) taken from the lane")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
