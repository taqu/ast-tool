#!/usr/bin/env python3
"""Task level3-005: log added before both AuditLogger::log calls in data_store.cpp."""
import sys
from pathlib import Path

LOG = "[audit] log called"
EXPECTED_MIN = 2

def main():
    target = Path("src/store/data_store.cpp")
    if not target.exists():
        print(f"FAIL: {target} not found")
        sys.exit(1)
    count = target.read_text(encoding="utf-8").count(LOG)
    if count < EXPECTED_MIN:
        print(f"FAIL: expected at least {EXPECTED_MIN} occurrences of '{LOG}' in data_store.cpp, found {count}")
        sys.exit(1)
    print(f"OK: found {count} occurrences")

if __name__ == "__main__":
    main()
