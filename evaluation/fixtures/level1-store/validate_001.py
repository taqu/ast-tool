#!/usr/bin/env python3
"""Task level1-001: [InventoryService] update log line added to implementation."""
import sys
from pathlib import Path

def main():
    target = Path("src/service/inventory_service.cpp")
    if not target.exists():
        print(f"FAIL: {target} not found")
        sys.exit(1)
    text = target.read_text(encoding="utf-8")
    if "[InventoryService] update" not in text:
        print("FAIL: log line '[InventoryService] update' not found in inventory_service.cpp")
        sys.exit(1)
    print("OK")

if __name__ == "__main__":
    main()
