#!/usr/bin/env python3
import sys
from pathlib import Path
import subprocess

def main():
    api_h = Path("src/api/api_handler.h")
    api_cpp = Path("src/api/api_handler.cpp")
    req_h = Path("src/processor/request_processor.h")
    req_cpp = Path("src/processor/request_processor.cpp")
    store_h = Path("src/store/database_store.h")
    store_cpp = Path("src/store/database_store.cpp")
    sync_h = Path("src/processor/sync_processor.h")
    main_cpp = Path("src/main.cpp")

    for f in (api_h, api_cpp, req_h, req_cpp, store_h, store_cpp, sync_h, main_cpp):
        if not f.exists():
            print(f"FAIL: {f} not found")
            sys.exit(1)

    # Check propagation in ApiHandler
    apih_content = api_h.read_text(encoding="utf-8")
    if "handleRequest(const std::string& req, const std::string& client_ip)" not in apih_content.replace(" ", ""):
        # Check if there is some signature with client_ip
        if "client_ip" not in apih_content:
            print("FAIL: ApiHandler::handleRequest signature not updated with client_ip")
            sys.exit(1)

    # Check propagation in RequestProcessor
    reqh_content = req_h.read_text(encoding="utf-8")
    if "client_ip" not in reqh_content:
        print("FAIL: RequestProcessor::process signature not updated with client_ip")
        sys.exit(1)

    # Check propagation in DatabaseStore
    storeh_content = store_h.read_text(encoding="utf-8")
    if "client_ip" not in storeh_content:
        print("FAIL: DatabaseStore::save signature not updated to accept client_ip")
        sys.exit(1)

    # Check SyncProcessor remains unchanged
    synch_content = sync_h.read_text(encoding="utf-8")
    if "client_ip" in synch_content:
        print("FAIL: SyncProcessor was incorrectly modified")
        sys.exit(1)

    # Check DatabaseStore prints ip
    storec_content = store_cpp.read_text(encoding="utf-8")
    if "ip=" not in storec_content:
        print("FAIL: DatabaseStore::save does not output client_ip")
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
