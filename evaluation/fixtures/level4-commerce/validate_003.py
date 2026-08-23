#!/usr/bin/env python3
import sys
from pathlib import Path
import subprocess

def main():
    gateway_h = Path("src/gateway/payment_gateway.h")
    gateway_cpp = Path("src/gateway/payment_gateway.cpp")
    service_h = Path("src/service/payment_service.h")
    service_cpp = Path("src/service/payment_service.cpp")
    order_cpp = Path("src/service/order_service.cpp")
    admin_cpp = Path("src/service/admin_order_service.cpp")

    for f in (gateway_h, gateway_cpp, service_h, service_cpp, order_cpp, admin_cpp):
        if not f.exists():
            print(f"FAIL: {f} not found")
            sys.exit(1)

    # Check payment gateway signature changes
    gh_content = gateway_h.read_text(encoding="utf-8")
    if "charge(const std::string& user_id, double amount, const std::string&" not in gh_content and "charge(const std::string& user_id, double amount, std::string " not in gh_content:
        print("FAIL: PaymentGateway::charge signature in header not updated with descriptor")
        sys.exit(1)

    # Check BillingGateway is unchanged
    if "BillingGateway" not in gh_content:
        print("FAIL: BillingGateway class is missing from header")
        sys.exit(1)
    if "class BillingGateway {\npublic:\n    bool charge(const std::string& user_id, double amount);" not in gh_content.replace("\r\n", "\n"):
        # Allow some spacing variations but ensure BillingGateway::charge still only takes 2 parameters
        lines = [line.strip() for line in gh_content.splitlines() if "charge" in line and "BillingGateway" not in line]
        billing_charge = [l for l in lines if "descriptor" not in l]
        if not billing_charge:
            print("FAIL: BillingGateway::charge signature was incorrectly modified")
            sys.exit(1)

    # Check payment service signature changes
    sh_content = service_h.read_text(encoding="utf-8")
    if "authorize(const std::string& user_id, double amount, const std::string&" not in sh_content and "authorize(const std::string& user_id, double amount, std::string " not in sh_content:
        print("FAIL: PaymentService::authorize signature in header not updated with descriptor")
        sys.exit(1)

    # Check implementations
    gc_content = gateway_cpp.read_text(encoding="utf-8")
    if "PaymentGateway::charge" not in gc_content:
        print("FAIL: PaymentGateway::charge not defined")
        sys.exit(1)
    if "BillingGateway::charge" not in gc_content:
        print("FAIL: BillingGateway::charge not defined")
        sys.exit(1)

    # Check callers passing string
    order_content = order_cpp.read_text(encoding="utf-8")
    if "authorize(user_id, 9.99 * quantity" not in order_content and "authorize(user_id, 9.99 * quantity, " not in order_content:
        print("FAIL: OrderService::submit call to authorize not updated")
        sys.exit(1)

    admin_content = admin_cpp.read_text(encoding="utf-8")
    if "authorize(admin_id, count * 5.0" not in admin_content and "authorize(admin_id, count * 5.0, " not in admin_content:
        print("FAIL: AdminOrderService::submit call to authorize not updated")
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
