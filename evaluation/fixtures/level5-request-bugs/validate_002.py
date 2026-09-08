#!/usr/bin/env python3
import sys
import subprocess
from pathlib import Path

def main():
    # Check compile and behavior
    cmd = ["clang++", "-std=c++17", "-Isrc", "test_002.cpp", "-o", "test_002"]
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print("FAIL: Compilation error")
        print(res.stderr)
        sys.exit(1)

    # Run check
    try:
        run_res = subprocess.run(["./test_002"], capture_output=True, text=True, timeout=5)
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
        for p in [Path("test_002"), Path("test_002.exe")]:
            if p.exists():
                try:
                    p.unlink()
                except:
                    pass

    print("OK")

if __name__ == "__main__":
    main()
