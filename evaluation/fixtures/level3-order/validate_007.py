#!/usr/bin/env python3
"""Task level3-007: trace log at entry of web handler, checkout service, and order service."""
import sys
from pathlib import Path

LOG = "[trace] checkout pipeline"

def main():
    targets = [
        Path("src/web/web_checkout_handler.cpp"),
        Path("src/service/checkout_service.cpp"),
        Path("src/service/order_service.cpp"),
    ]
    for f in targets:
        if not f.exists():
            print(f"FAIL: {f} not found")
            sys.exit(1)
        if LOG not in f.read_text(encoding="utf-8"):
            print(f"FAIL: '{LOG}' not found in {f.name}")
            sys.exit(1)
    print("OK")

if __name__ == "__main__":
    main()
