#!/usr/bin/env python3
"""Task level3-006: log added before store_.save() call in sync_job.cpp."""
import sys
from pathlib import Path

LOG = "[sync] persisting"

def main():
    target = Path("src/job/sync_job.cpp")
    if not target.exists():
        print(f"FAIL: {target} not found")
        sys.exit(1)
    if LOG not in target.read_text(encoding="utf-8"):
        print(f"FAIL: '{LOG}' not found in sync_job.cpp")
        sys.exit(1)
    print("OK")

if __name__ == "__main__":
    main()
