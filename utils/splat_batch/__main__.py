"""Allow ``python3 -m splat_batch`` (run from the utils directory)."""

import sys

from .cli import main

if __name__ == "__main__":
    sys.exit(main())
