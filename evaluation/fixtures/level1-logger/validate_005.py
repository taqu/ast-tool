#!/usr/bin/env python3
"""Task level1-005: log line added before every Connection::send call in server.cpp."""
import sys
from pathlib import Path

LOG = "[server] send"
EXPECTED_MIN = 2

def main():
    target = Path("src/net/server.cpp")
    if not target.exists():
        print(f"FAIL: {target} not found")
        sys.exit(1)
    count = target.read_text(encoding="utf-8").count(LOG)
    if count < EXPECTED_MIN:
        print(f"FAIL: expected at least {EXPECTED_MIN} occurrences of '{LOG}' in server.cpp, found {count}")
        sys.exit(1)
    print(f"OK: found {count} occurrences")

if __name__ == "__main__":
    main()
