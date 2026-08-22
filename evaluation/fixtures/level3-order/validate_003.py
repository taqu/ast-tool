#!/usr/bin/env python3
"""Task level3-003: log at start of InventoryRepository::reserve, not OrderRepository."""
import sys
from pathlib import Path

LOG = "[inventory] persisting"

def main():
    inv = Path("src/repository/inventory_repository.cpp")
    order = Path("src/repository/order_repository.cpp")

    if not inv.exists():
        print(f"FAIL: {inv} not found")
        sys.exit(1)
    if LOG not in inv.read_text(encoding="utf-8"):
        print(f"FAIL: '{LOG}' not found in inventory_repository.cpp")
        sys.exit(1)
    if order.exists() and LOG in order.read_text(encoding="utf-8"):
        print(f"FAIL: '{LOG}' was incorrectly added to order_repository.cpp")
        sys.exit(1)

    print("OK")

if __name__ == "__main__":
    main()
