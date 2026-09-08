#!/usr/bin/env python3
"""Task level2-005: [repo] update log in UserRepository::update."""
import sys
from pathlib import Path

LOG = "[repo] update"

def main():
    target = Path("src/auth/user_repository.cpp")
    if not target.exists():
        print(f"FAIL: {target} not found")
        sys.exit(1)
    if LOG not in target.read_text(encoding="utf-8"):
        print(f"FAIL: '{LOG}' not found in user_repository.cpp")
        sys.exit(1)
    print("OK")

if __name__ == "__main__":
    main()
