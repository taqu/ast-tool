#!/usr/bin/env python3
import sys
from pathlib import Path
import subprocess

def main():
    registry_h = Path("src/api/callback_registry.h")
    registry_cpp = Path("src/api/callback_registry.cpp")
    main_cpp = Path("src/main.cpp")

    for f in (registry_h, registry_cpp, main_cpp):
        if not f.exists():
            print(f"FAIL: {f} not found")
            sys.exit(1)

    # Check RequestHandlerCallback signature change
    rh_content = registry_h.read_text(encoding="utf-8")
    if "std::function<bool" not in rh_content.replace(" ", ""):
        print("FAIL: RequestHandlerCallback signature in callback_registry.h not changed to return bool")
        sys.exit(1)

    # Check CallbackRegistry::trigger update
    rc_content = registry_cpp.read_text(encoding="utf-8")
    expected_check = "[callback] failure reported for"
    if expected_check not in rc_content:
        print("FAIL: CallbackRegistry::trigger does not log callback failures")
        sys.exit(1)
    if "if (!cb" not in rc_content.replace(" ", "") and "if(!cb" not in rc_content.replace(" ", ""):
        print("FAIL: CallbackRegistry::trigger does not check return value of callback")
        sys.exit(1)

    # Check main.cpp lambda return value
    m_content = main_cpp.read_text(encoding="utf-8")
    if "return true;" not in m_content and "return false;" not in m_content:
        print("FAIL: Callback lambda in main.cpp does not return a boolean value")
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
