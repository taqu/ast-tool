#!/usr/bin/env python3
"""Task level2-008: [auth] token expired on logout in handleLogout only."""
import sys
from pathlib import Path

LOG = "[auth] token expired on logout"

def main():
    ctrl_file = Path("src/web/auth_controller.cpp")
    if not ctrl_file.exists():
        print(f"FAIL: {ctrl_file} not found")
        sys.exit(1)
    if LOG not in ctrl_file.read_text(encoding="utf-8"):
        print(f"FAIL: '{LOG}' not found in auth_controller.cpp")
        sys.exit(1)
    svc_file = Path("src/auth/auth_service.cpp")
    if svc_file.exists() and LOG in svc_file.read_text(encoding="utf-8"):
        print(f"FAIL: '{LOG}' was incorrectly added to auth_service.cpp")
        sys.exit(1)
    print("OK")

if __name__ == "__main__":
    main()
