#!/usr/bin/env python3
"""Task level1-004: guard added before inventory lookup in OrderService::process."""
import sys
from pathlib import Path

def main():
    target = Path("src/service/order_service.cpp")
    if not target.exists():
        print(f"FAIL: {target} not found")
        sys.exit(1)
    text = target.read_text(encoding="utf-8")
    if "[OrderService] invalid product_id" not in text:
        print("FAIL: log line '[OrderService] invalid product_id' not found in order_service.cpp")
        sys.exit(1)
    if "return false" not in text:
        print("FAIL: 'return false' guard not found in order_service.cpp")
        sys.exit(1)
    print("OK")

if __name__ == "__main__":
    main()
