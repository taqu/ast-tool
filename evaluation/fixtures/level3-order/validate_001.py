#!/usr/bin/env python3
"""Task level3-001: log added to web and mobile handlers, not to retry worker."""
import sys
from pathlib import Path

LOG = "[handler] checkout start"

def main():
    web = Path("src/web/web_checkout_handler.cpp")
    mobile = Path("src/web/mobile_checkout_handler.cpp")
    retry = Path("src/worker/retry_worker.cpp")

    for f in (web, mobile):
        if not f.exists():
            print(f"FAIL: {f} not found")
            sys.exit(1)
        if LOG not in f.read_text(encoding="utf-8"):
            print(f"FAIL: '{LOG}' not found in {f}")
            sys.exit(1)

    if retry.exists() and LOG in retry.read_text(encoding="utf-8"):
        print(f"FAIL: '{LOG}' was incorrectly added to retry_worker.cpp")
        sys.exit(1)

    print("OK")

if __name__ == "__main__":
    main()
