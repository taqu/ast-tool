"""Compare the monolithic C++ runner on Phase 8a and Phase 8b source."""
from __future__ import annotations

import json
import os
from pathlib import Path
import re
import subprocess


REPO = Path(__file__).resolve().parent.parent
SOURCES = [
    REPO / "CMakeLists.txt",
    REPO / "src/ast-callers.cpp",
    REPO / "src/ast-callees.cpp",
    REPO / "src/ast-references.cpp",
]
TEST_BINARY = REPO / "test/bin/ast-tool-test.exe"
OUT = REPO / "evaluation/phase8b/regression-comparison.json"


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
    for cache in (REPO / "test").glob("**/.ast-tool/ast-cache.db"):
        cache.unlink()
    result = subprocess.run(
        [str(TEST_BINARY)], cwd=REPO, capture_output=True, text=True, encoding="utf-8"
    )
    combined = result.stdout + result.stderr
    failures = re.findall(r"^\s*FAIL:\s*(.+)$", combined, re.MULTILINE)
    return {
        "returncode": result.returncode,
        "failure_count": len(failures),
        "failures": failures,
    }


def run_repeated(count: int = 3) -> dict[str, object]:
    runs = [run_test() for _ in range(count)]
    systematic = set(runs[0]["failures"])
    for run in runs[1:]:
        systematic &= set(run["failures"])
    return {"runs": runs, "systematic_failures": sorted(systematic)}


def main() -> None:
    phase8b = {path: path.read_bytes() for path in SOURCES}
    try:
        for path in SOURCES:
            relative = path.relative_to(REPO).as_posix()
            path.write_bytes(subprocess.check_output(["git", "show", f"HEAD:{relative}"], cwd=REPO))
        build_test()
        baseline = run_repeated()
    finally:
        for path, content in phase8b.items():
            path.write_bytes(content)

    build_test()
    current = run_repeated()
    baseline_systematic = set(baseline["systematic_failures"])
    current_systematic = set(current["systematic_failures"])
    payload = {
        "phase8a_baseline": baseline,
        "phase8b": current,
        "unchanged_systematic_failures": sorted(baseline_systematic & current_systematic),
        "resolved_systematic_failures": sorted(baseline_systematic - current_systematic),
        "new_systematic_failures": sorted(current_systematic - baseline_systematic),
    }
    OUT.write_text(json.dumps(payload, indent=2), encoding="utf-8")
    print(json.dumps(payload, indent=2))


if __name__ == "__main__":
    main()
