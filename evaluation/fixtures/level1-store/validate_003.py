#!/usr/bin/env python3
"""Task level1-003: guard added to OrderService::save, not InventoryService::save."""
import sys
from pathlib import Path

def main():
    order_file = Path("src/service/order_service.cpp")
    inv_file = Path("src/service/inventory_service.cpp")

    if not order_file.exists():
        print(f"FAIL: {order_file} not found")
        sys.exit(1)

    order_text = order_file.read_text(encoding="utf-8")

    has_guard = "product.id == 0" in order_text or "product.id==0" in order_text
    if not has_guard:
        print("FAIL: zero-ID guard not found in order_service.cpp")
        sys.exit(1)

    if "[OrderService] invalid id" not in order_text:
        print("FAIL: log line '[OrderService] invalid id' not found in order_service.cpp")
        sys.exit(1)

    inv_text = inv_file.read_text(encoding="utf-8")
    if "product.id == 0" in inv_text or "product.id==0" in inv_text:
        print("FAIL: guard was incorrectly added to inventory_service.cpp")
        sys.exit(1)

    print("OK")

if __name__ == "__main__":
    main()
