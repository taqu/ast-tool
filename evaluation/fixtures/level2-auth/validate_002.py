#!/usr/bin/env python3
"""Task level2-002: [AuthToken] validate log in auth::AuthToken::validate only."""
import sys
from pathlib import Path

LOG = "[AuthToken] validate"

def main():
    token_file = Path("src/auth/auth_token.cpp")
    if not token_file.exists():
        print(f"FAIL: {token_file} not found")
        sys.exit(1)
    if LOG not in token_file.read_text(encoding="utf-8"):
        print(f"FAIL: '{LOG}' not found in auth_token.cpp")
        sys.exit(1)
    service_file = Path("src/auth/auth_service.cpp")
    if service_file.exists() and LOG in service_file.read_text(encoding="utf-8"):
        print(f"FAIL: '{LOG}' was incorrectly added to auth_service.cpp")
        sys.exit(1)
    print("OK")

if __name__ == "__main__":
    main()
