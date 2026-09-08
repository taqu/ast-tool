#!/usr/bin/env python3
"""Task level3-002: log before PaymentService::authorize call in order_service only."""
import sys
from pathlib import Path

LOG = "[order] authorizing payment"

def main():
    order = Path("src/service/order_service.cpp")
    retry = Path("src/worker/retry_worker.cpp")

    if not order.exists():
        print(f"FAIL: {order} not found")
        sys.exit(1)
    if LOG not in order.read_text(encoding="utf-8"):
        print(f"FAIL: '{LOG}' not found in order_service.cpp")
        sys.exit(1)
    if retry.exists() and LOG in retry.read_text(encoding="utf-8"):
        print(f"FAIL: '{LOG}' was incorrectly added to retry_worker.cpp")
        sys.exit(1)

    print("OK")

if __name__ == "__main__":
    main()
