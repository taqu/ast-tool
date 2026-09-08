#!/usr/bin/env python3
import sys
import subprocess
from pathlib import Path

def main():
    workflow_cpp = Path("src/workflow/password_reset_workflow.cpp")
    user_cpp = Path("src/service/user_service.cpp")

    if not workflow_cpp.exists():
        print(f"FAIL: {workflow_cpp} not found")
        sys.exit(1)

    cpp_files = [
        "src/service/notification_service.cpp",
        "src/service/user_service.cpp",
        "src/workflow/password_reset_workflow.cpp"
    ]

    # Compile check
    cmd = ["clang++", "-std=c++17", "-Isrc"] + cpp_files + ["test_008.cpp", "-o", "test_008"]
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print("FAIL: Compilation error")
        print(res.stderr)
        sys.exit(1)

    # Run check
    try:
        run_res = subprocess.run(["./test_008"], capture_output=True, text=True, timeout=5)
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
        for p in [Path("test_008"), Path("test_008.exe")]:
            if p.exists():
                try:
                    p.unlink()
                except:
                    pass

    # Ensure UserService::change_password was NOT modified to remove email (regression boundary)
    user_content = user_cpp.read_text(encoding="utf-8")
    if "send_email" not in user_content:
        print("FAIL: UserService::change_password should still send email notification directly")
        sys.exit(1)

    print("OK")

if __name__ == "__main__":
    main()
