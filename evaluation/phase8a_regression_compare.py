"""Compare the monolithic C++ test runner before and after the Phase 8a resolver."""
from __future__ import annotations

import json
import os
from pathlib import Path
import re
import subprocess


REPO = Path(__file__).resolve().parent.parent
SOURCE = REPO / "src" / "cli-semantic.cpp"
TEST_BINARY = REPO / "test" / "bin" / "ast-tool-test.exe"
OUT = REPO / "evaluation" / "phase8a" / "regression-comparison.json"


def build_environment() -> dict[str, str]:
    environment = {key: value for key, value in os.environ.items() if key.lower() != "path"}
    environment["Path"] = os.environ["PATH"]
    return environment


def build_test() -> None:
    subprocess.run(
        ["cmake", "--build", "build", "--config", "Release", "--target", "ast-tool-test"],
        cwd=REPO, env=build_environment(), check=True, capture_output=True, text=True,
    )


def run_test() -> dict[str, object]:
    cache = REPO / ".ast-tool" / "ast-cache.db"
    if cache.exists():
        cache.unlink()
    result = subprocess.run(
        [str(TEST_BINARY)], cwd=REPO, capture_output=True, text=True, encoding="utf-8"
    )
    combined = result.stdout + result.stderr
    failures = re.findall(r"^\s*FAIL:\s*(.+)$", combined, re.MULTILINE)
    phase8a_results = [
        line.strip() for line in combined.splitlines()
        if "resolver: qualified suffix" in line or "unique suffix works" in line
    ]
    return {
        "returncode": result.returncode,
        "failure_count": len(failures),
        "failures": failures,
        "phase8a_results": phase8a_results,
    }


def main() -> None:
    phase8a_source = SOURCE.read_bytes()
    baseline_source = subprocess.check_output(
        ["git", "show", "HEAD:src/cli-semantic.cpp"], cwd=REPO
    )
    try:
        SOURCE.write_bytes(baseline_source)
        build_test()
        baseline = run_test()
    finally:
        SOURCE.write_bytes(phase8a_source)

    build_test()
    phase8a = run_test()
    payload = {
        "baseline": baseline,
        "phase8a": phase8a,
        "unchanged_existing_failures": sorted(set(baseline["failures"]) & set(phase8a["failures"])),
        "new_non_phase8a_failures": sorted(set(phase8a["failures"]) - set(baseline["failures"])),
    }
    OUT.write_text(json.dumps(payload, indent=2), encoding="utf-8")
    print(json.dumps(payload, indent=2))


if __name__ == "__main__":
    main()
