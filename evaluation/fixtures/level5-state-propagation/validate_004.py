#!/usr/bin/env python3
import sys
import subprocess
from pathlib import Path

def main():
    user_cpp = Path("src/service/user_service.cpp")
    if not user_cpp.exists():
        print(f"FAIL: {user_cpp} not found")
        sys.exit(1)

    cpp_files = [
        "src/service/user_service.cpp",
        "src/service/transaction_manager.cpp"
    ]

    # Compile check
    cmd = ["clang++", "-std=c++17", "-Isrc"] + cpp_files + ["test_004.cpp", "-o", "test_004"]
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print("FAIL: Compilation error")
        print(res.stderr)
        sys.exit(1)

    # Run check
    try:
        run_res = subprocess.run(["./test_004"], capture_output=True, text=True, timeout=5)
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
        for p in [Path("test_004"), Path("test_004.exe")]:
            if p.exists():
                try:
                    p.unlink()
                except:
                    pass

    print("OK")

if __name__ == "__main__":
    main()
