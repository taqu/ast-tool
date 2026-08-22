#!/usr/bin/env python3
"""Task level2-001: [AuthService] update log in auth::AuthService::update only."""
import sys
from pathlib import Path

LOG = "[AuthService] update"

def main():
    target = Path("src/auth/auth_service.cpp")
    if not target.exists():
        print(f"FAIL: {target} not found")
        sys.exit(1)
    if LOG not in target.read_text(encoding="utf-8"):
        print(f"FAIL: '{LOG}' not found in auth_service.cpp")
        sys.exit(1)
    for other in [
        "src/session/session_manager.cpp",
        "src/auth/token_cache.cpp",
        "src/auth/user_repository.cpp",
    ]:
        p = Path(other)
        if p.exists() and LOG in p.read_text(encoding="utf-8"):
            print(f"FAIL: '{LOG}' unexpectedly found in {other}")
            sys.exit(1)
    print("OK")

if __name__ == "__main__":
    main()
