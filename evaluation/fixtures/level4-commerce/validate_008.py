#!/usr/bin/env python3
import sys
from pathlib import Path
import subprocess

def main():
    inv_h = Path("src/service/inventory_service.h")
    inv_cpp = Path("src/service/inventory_service.cpp")
    order_cpp = Path("src/service/order_service.cpp")
    admin_cpp = Path("src/service/admin_order_service.cpp")

    for f in (inv_h, inv_cpp, order_cpp, admin_cpp):
        if not f.exists():
            print(f"FAIL: {f} not found")
            sys.exit(1)

    # Check InventoryService update
    ih_content = inv_h.read_text(encoding="utf-8")
    if "checkAvailability" not in ih_content:
        print("FAIL: checkAvailability not declared in inventory_service.h")
        sys.exit(1)

    ic_content = inv_cpp.read_text(encoding="utf-8")
    if "checkAvailability" not in ic_content:
        print("FAIL: checkAvailability not defined in inventory_service.cpp")
        sys.exit(1)
    if "available(" not in ic_content:
        print("FAIL: checkAvailability does not call repository available()")
        sys.exit(1)

    # Check OrderService update
    oc_content = order_cpp.read_text(encoding="utf-8")
    expected_log = "[inventory] insufficient stock for "
    if expected_log not in oc_content:
        print("FAIL: Insufficient stock warning log not found in order_service.cpp")
        sys.exit(1)
    if "checkAvailability" not in oc_content:
        print("FAIL: OrderService::submit does not check inventory availability")
        sys.exit(1)

    # Check AdminOrderService remains untouched
    ac_content = admin_cpp.read_text(encoding="utf-8")
    if "checkAvailability" in ac_content:
        print("FAIL: AdminOrderService was incorrectly updated with availability check")
        sys.exit(1)

    # Compile check
    cpp_files = [
        "src/gateway/payment_gateway.cpp",
        "src/repository/admin_repository.cpp",
        "src/repository/inventory_repository.cpp",
        "src/repository/order_repository.cpp",
        "src/service/admin_order_service.cpp",
        "src/service/checkout_service.cpp",
        "src/service/inventory_service.cpp",
        "src/service/notification_service.cpp",
        "src/service/order_service.cpp",
        "src/service/payment_service.cpp"
    ]
    
    cmd = ["clang++", "-std=c++17", "-Isrc", "-fsyntax-only"] + cpp_files
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print("FAIL: Compilation error")
        print(res.stderr)
        sys.exit(1)

    print("OK")

if __name__ == "__main__":
    main()
