#!/usr/bin/env python3
"""Task level2-003: log added only to process(const Request&) overload."""
import sys
from pathlib import Path

LOG = "[handler] process request"

def main():
    target = Path("src/gateway/request_handler.cpp")
    if not target.exists():
        print(f"FAIL: {target} not found")
        sys.exit(1)
    count = target.read_text(encoding="utf-8").count(LOG)
    if count != 1:
        print(f"FAIL: expected exactly 1 occurrence of '{LOG}', found {count}")
        sys.exit(1)
    print("OK")

if __name__ == "__main__":
    main()
