#!/usr/bin/env python3
import sys
import subprocess
from pathlib import Path

def main():
    job_cpp = Path("src/jobs/subscription_billing_job.cpp")
    payment_cpp = Path("src/service/payment_service.h")

    if not job_cpp.exists():
        print(f"FAIL: {job_cpp} not found")
        sys.exit(1)

    # Compile check
    cmd = ["clang++", "-std=c++17", "-Isrc", "src/jobs/subscription_billing_job.cpp", "test_005.cpp", "-o", "test_005"]
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print("FAIL: Compilation error")
        print(res.stderr)
        sys.exit(1)

    # Run check
    try:
        run_res = subprocess.run(["./test_005"], capture_output=True, text=True, timeout=5)
        if run_res.returncode != 0:
            print("FAIL: Test execution failed")
            print(run_res.stdout)
            print(run_res.stderr)
            sys.exit(1)
        if "SUCCESS" not in run_res.stdout:
            print("FAIL: Execution output mismatch")
            sys.exit(1)
    finally:
        # cleanup
        for p in [Path("test_005"), Path("test_005.exe")]:
            if p.exists():
                try:
                    p.unlink()
                except:
                    pass

    # Ensure PaymentService::charge was NOT modified (regression boundary)
    payment_content = payment_cpp.read_text(encoding="utf-8")
    if "double amount" not in payment_content:
        print("FAIL: PaymentService::charge signature was modified")
        sys.exit(1)

    print("OK")

if __name__ == "__main__":
    main()
