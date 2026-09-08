#!/usr/bin/env python3
"""Task level1-008: error handling added when config load fails in Application::init."""
import sys
from pathlib import Path

def main():
    target = Path("src/app/application.cpp")
    if not target.exists():
        print(f"FAIL: {target} not found")
        sys.exit(1)
    text = target.read_text(encoding="utf-8")
    if "[Application] load failed" not in text:
        print("FAIL: log line '[Application] load failed' not found in application.cpp")
        sys.exit(1)
    if "return false" not in text:
        print("FAIL: 'return false' not found in application.cpp")
        sys.exit(1)
    print("OK")

if __name__ == "__main__":
    main()
