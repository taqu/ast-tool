#!/usr/bin/env python3
"""Task level1-002: log line added before every InventoryService::save call site."""
import sys
from pathlib import Path

LOG = "[log] inventory save"
EXPECTED_MIN = 3

def main():
    cpp_files = list(Path("src").rglob("*.cpp"))
    count = 0
    for f in cpp_files:
        count += f.read_text(encoding="utf-8").count(LOG)
    if count < EXPECTED_MIN:
        print(f"FAIL: expected at least {EXPECTED_MIN} occurrences of '{LOG}', found {count}")
        sys.exit(1)
    print(f"OK: found {count} occurrences of log line")

if __name__ == "__main__":
    main()
