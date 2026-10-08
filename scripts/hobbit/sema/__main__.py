"""`python3 -m hobbit.sema <view> ...` - the same dispatch `hobbit sema` uses."""

from __future__ import annotations

import sys

from hobbit.sema import main

if __name__ == "__main__":
    sys.exit(main())
