#!/usr/bin/env python3
import sys
from pathlib import Path
import subprocess

def main():
    order_svc = Path("src/service/order_service.cpp")
    admin_svc = Path("src/service/admin_order_service.cpp")

    if not order_svc.exists():
        print(f"FAIL: {order_svc} not found")
        sys.exit(1)

    order_content = order_svc.read_text(encoding="utf-8")
    expected_log = '[audit] order submitted for '
    
    if expected_log not in order_content:
        print(f"FAIL: Expected audit log not found in {order_svc}")
        sys.exit(1)

    if admin_svc.exists():
        admin_content = admin_svc.read_text(encoding="utf-8")
        if "[audit]" in admin_content:
            print(f"FAIL: Audit log incorrectly added to {admin_svc}")
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
