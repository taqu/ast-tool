#!/usr/bin/env python3
import sys
from pathlib import Path
import subprocess

def main():
    req_processor = Path("src/processor/request_processor.cpp")
    sync_processor = Path("src/processor/sync_processor.cpp")

    if not req_processor.exists():
        print(f"FAIL: {req_processor} not found")
        sys.exit(1)

    content = req_processor.read_text(encoding="utf-8")
    expected_log = '[security] restricted payload blocked for request '
    
    if expected_log not in content:
        print(f"FAIL: Expected security log not found in {req_processor}")
        sys.exit(1)
        
    if "return false;" not in content:
        print(f"FAIL: Expected request processor to abort and return false")
        sys.exit(1)

    if sync_processor.exists():
        sync_content = sync_processor.read_text(encoding="utf-8")
        if "[security]" in sync_content:
            print(f"FAIL: Security log incorrectly added to {sync_processor}")
            sys.exit(1)

    # Compile check
    cpp_files = [
        "src/main.cpp",
        "src/api/api_handler.cpp",
        "src/api/callback_registry.cpp",
        "src/jobs/cron_job.cpp",
        "src/processor/request_processor.cpp",
        "src/processor/sync_processor.cpp",
        "src/service/enrichment_service.cpp",
        "src/service/validation_service.cpp",
        "src/store/database_store.cpp"
    ]
    
    cmd = ["clang++", "-std=c++17", "-Isrc", "-fsyntax-only"] + cpp_files
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print("FAIL: Compilation error")
        print(res.stderr)
        sys.exit(1)

    print("OK")

if __name__ == "__main__":
    main()
