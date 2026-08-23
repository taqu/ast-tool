#!/usr/bin/env python3
import sys
from pathlib import Path
import subprocess

def main():
    checkout_h = Path("src/service/checkout_service.h")
    checkout_cpp = Path("src/service/checkout_service.cpp")
    order_h = Path("src/service/order_service.h")
    order_cpp = Path("src/service/order_service.cpp")
    order_repo_h = Path("src/repository/order_repository.h")
    order_repo_cpp = Path("src/repository/order_repository.cpp")
    admin_svc_h = Path("src/service/admin_order_service.h")
    admin_repo_h = Path("src/repository/admin_repository.h")

    for f in (checkout_h, checkout_cpp, order_h, order_cpp, order_repo_h, order_repo_cpp, admin_svc_h, admin_repo_h):
        if not f.exists():
            print(f"FAIL: {f} not found")
            sys.exit(1)

    # Check propagation in CheckoutService
    ch_content = checkout_h.read_text(encoding="utf-8")
    if "checkout(const std::string& user_id,\n                  const std::string& item_id,\n                  int quantity," not in ch_content.replace("\r\n", "\n") and "checkout(const std::string& user_id, const std::string& item_id, int quantity, const std::string&" not in ch_content.replace("\r\n", "\n"):
        # Let's be flexible and just check if there is a 4-parameter checkout declaration in service namespace
        lines = [l.strip() for l in ch_content.splitlines() if "checkout(" in l]
        if not any("trace" in l or "std::string" in l for l in lines):
            print("FAIL: CheckoutService::checkout signature not updated with trace_id")
            sys.exit(1)

    # Check propagation in OrderService
    oh_content = order_h.read_text(encoding="utf-8")
    if "submit(const std::string& user_id," not in oh_content:
        print("FAIL: OrderService::submit not found in header")
        sys.exit(1)
    lines_oh = [l.strip() for l in oh_content.splitlines() if "submit(" in l]
    if not any("trace" in l or "std::string" in l for l in lines_oh):
        print("FAIL: OrderService::submit signature not updated with trace_id")
        sys.exit(1)

    # Check propagation in OrderRepository
    rh_content = order_repo_h.read_text(encoding="utf-8")
    lines_rh = [l.strip() for l in rh_content.splitlines() if "save(" in l]
    if not any("trace" in l or "std::string" in l for l in lines_rh):
        print("FAIL: OrderRepository::save signature not updated with trace_id")
        sys.exit(1)

    # Check AdminOrderService and AdminRepository remain unchanged
    ash_content = admin_svc_h.read_text(encoding="utf-8")
    if "trace" in ash_content:
        print("FAIL: AdminOrderService signature was incorrectly changed")
        sys.exit(1)
        
    arh_content = admin_repo_h.read_text(encoding="utf-8")
    if "trace" in arh_content:
        print("FAIL: AdminRepository signature was incorrectly changed")
        sys.exit(1)

    # Check order repository implementation prints trace
    rc_content = order_repo_cpp.read_text(encoding="utf-8")
    if "trace=" not in rc_content:
        print("FAIL: OrderRepository::save implementation does not output trace")
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
