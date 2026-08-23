#!/usr/bin/env python3
import sys
from pathlib import Path
import subprocess

def main():
    val_h = Path("src/service/validation_service.h")
    val_cpp = Path("src/service/validation_service.cpp")
    req_cpp = Path("src/processor/request_processor.cpp")
    store_h = Path("src/store/database_store.h")
    job_h = Path("src/jobs/cron_job.h")

    for f in (val_h, val_cpp, req_cpp, store_h, job_h):
        if not f.exists():
            print(f"FAIL: {f} not found")
            sys.exit(1)

    # Check validation service rename
    vh_content = val_h.read_text(encoding="utf-8")
    if "validatePayload" not in vh_content:
        print("FAIL: validatePayload not found in validation_service.h")
        sys.exit(1)
    if "bool validate(" in vh_content:
        print("FAIL: Old validate method still exists in validation_service.h")
        sys.exit(1)

    vc_content = val_cpp.read_text(encoding="utf-8")
    if "ValidationService::validatePayload" not in vc_content:
        print("FAIL: ValidationService::validatePayload not defined in validation_service.cpp")
        sys.exit(1)

    # Check request processor caller update
    rc_content = req_cpp.read_text(encoding="utf-8")
    if "validatePayload(" not in rc_content:
        print("FAIL: RequestProcessor did not update call to validatePayload")
        sys.exit(1)
    if "validator_.validate(" in rc_content:
        print("FAIL: RequestProcessor still calling old validate method")
        sys.exit(1)

    # Check store and jobs remain unchanged
    sh_content = store_h.read_text(encoding="utf-8")
    if "validate" not in sh_content:
        print("FAIL: DatabaseStore::validate is missing")
        sys.exit(1)
    if "validatePayload" in sh_content:
        print("FAIL: DatabaseStore::validate was incorrectly renamed")
        sys.exit(1)

    jh_content = job_h.read_text(encoding="utf-8")
    if "validate" not in jh_content:
        print("FAIL: CronJob::validate is missing")
        sys.exit(1)
    if "validatePayload" in jh_content:
        print("FAIL: CronJob::validate was incorrectly renamed")
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
