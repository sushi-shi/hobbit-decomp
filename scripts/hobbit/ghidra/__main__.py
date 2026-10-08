"""`python3 -m hobbit.ghidra <verb> ...` - the same dispatch `hobbit ghidra` uses."""

from __future__ import annotations

import sys

from hobbit.ghidra import main

if __name__ == "__main__":
    sys.exit(main())
