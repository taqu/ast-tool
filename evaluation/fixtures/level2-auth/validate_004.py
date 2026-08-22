#!/usr/bin/env python3
"""Task level2-004: log added before every direct AuthToken::validate call site."""
import sys
from pathlib import Path

LOG = "[auth] validating token"
EXPECTED_MIN = 4

def main():
    count = 0
    for f in Path("src").rglob("*.cpp"):
        count += f.read_text(encoding="utf-8").count(LOG)
    if count < EXPECTED_MIN:
        print(f"FAIL: expected at least {EXPECTED_MIN} occurrences of '{LOG}', found {count}")
        sys.exit(1)
    print(f"OK: found {count} occurrences")

if __name__ == "__main__":
    main()
