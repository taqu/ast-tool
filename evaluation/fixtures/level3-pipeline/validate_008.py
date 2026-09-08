#!/usr/bin/env python3
"""Task level3-008: log before audit-triggering store calls in sync and cleanup jobs, not report job."""
import sys
from pathlib import Path

LOG = "[job] audit triggered"

def main():
    sync = Path("src/job/sync_job.cpp")
    cleanup = Path("src/job/cleanup_job.cpp")
    report = Path("src/job/report_job.cpp")

    for f in (sync, cleanup):
        if not f.exists():
            print(f"FAIL: {f} not found")
            sys.exit(1)
        if LOG not in f.read_text(encoding="utf-8"):
            print(f"FAIL: '{LOG}' not found in {f.name}")
            sys.exit(1)

    if report.exists() and LOG in report.read_text(encoding="utf-8"):
        print(f"FAIL: '{LOG}' was incorrectly added to report_job.cpp")
        sys.exit(1)

    print("OK")

if __name__ == "__main__":
    main()
