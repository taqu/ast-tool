#!/usr/bin/env python3
"""Task level3-004: log at start of PaymentGateway::charge."""
import sys
from pathlib import Path

LOG = "[gateway] charge"

def main():
    target = Path("src/gateway/payment_gateway.cpp")
    if not target.exists():
        print(f"FAIL: {target} not found")
        sys.exit(1)
    if LOG not in target.read_text(encoding="utf-8"):
        print(f"FAIL: '{LOG}' not found in payment_gateway.cpp")
        sys.exit(1)
    print("OK")

if __name__ == "__main__":
    main()
