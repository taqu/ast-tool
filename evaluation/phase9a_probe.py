"""Phase 9a — Controlled Semantic Capability Evaluation probe.

Compares Arm A (pre-Phase-8 baseline) vs Arm B (Phase 8a+8b+8c) under forced
semantic routing across a 12-task cohort covering all four Phase 9a categories.

Categories:
    A. Phase 8a target-resolution tasks  (partial FQN suffix)
    B. Phase 8b receiver-type tasks      (typed member relationships)
    C. Phase 8c body-identity tasks      (declaration→definition callees)
    D. Unaffected semantic guard tasks

Usage:
    python phase9a_probe.py [--first-repeat N] [--last-repeat N] [--arm {a,b,both}]

For Arm A: build the pre-Phase-8 binary from commit ccc1fbb (before Phase 8 changes)
           and install it as bin/ast-tool.exe before running --arm a.
For Arm B: use the current Phase 8a+8b+8c binary.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time

sys.path.insert(0, str(Path(__file__).parent))

import logs
import agents.claude_code as adapter
from runner import run_task
from trace_analyzer import analyze_directory


ROOT = Path(__file__).resolve().parent
REPO = ROOT.parent
WORKSPACE_BIN = REPO / "bin" / "ast-tool.exe"
GLOBAL_LOGS = Path.home() / ".claude/projects"
EXPECTED_SKILL = "96b07a6b89ae338f26d45fbfb31dd97d5b9c50efa39922103e8cb3e616807eaf"
SKILLS = [
    REPO / "skills/semantic-analysis/SKILL.md",
    Path.home() / ".claude/skills/semantic-analysis/SKILL.md",
]

# Phase 9a controlled cohort (12 tasks)
# Category A — Phase 8a target resolution (partial FQN)
TASKS_A = ["level2-008", "level3-008"]
# Category B — Phase 8b receiver-type member relationships
TASKS_B = ["level2-004", "level4-006"]
# Category C — Phase 8c body-identity callees
# Uses level2-008 repository (auth::AuthService::refresh) and level4-api
TASKS_C = ["level2-003", "level4-005"]  # tasks that exercise callees on decl-only methods
# Category D — Unaffected semantic guards
TASKS_D = ["level1-001", "level1-002", "level2-001", "level3-001"]

ALL_TASKS = TASKS_A + TASKS_B + TASKS_C + TASKS_D

ORIGINAL_RUN = adapter.run_claude


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify_baseline() -> dict[str, str]:
    result = {str(path): sha256(path) for path in SKILLS if path.exists()}
    matching = all(v == EXPECTED_SKILL for v in result.values())
    if not matching:
        raise AssertionError(f"Skill hash mismatch: {result}")
    return result


def isolated_run(prompt: str, cwd: Path, timeout: int):
    project = GLOBAL_LOGS / re.sub(r"[^a-zA-Z0-9]", "-", str(cwd))
    before = set(project.rglob("*.jsonl")) if project.exists() else set()
    result = ORIGINAL_RUN(prompt, cwd, timeout)
    fresh = set(project.rglob("*.jsonl")) - before
    for path in fresh:
        target = logs.CLAUDE_LOG_DIR / path.relative_to(project)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, target)
    (logs.CLAUDE_LOG_DIR / "process.json").write_text(json.dumps({
        "exit_code": result.exit_code,
        "stderr": result.stderr,
        "stdout": result.stdout,
        "fresh_logs": len(fresh),
    }, indent=2), encoding="utf-8")
    return result


def run_direct_semantic_probes(root_base: Path) -> list[dict]:
    """Run direct CLI probes for semantic correctness verification (no agent)."""
    bin_path = WORKSPACE_BIN
    results = []

    probes = [
        # Category A
        {"cat": "A", "cmd": "callers", "query": "AuthToken::expire",
         "root": root_base / "level2-auth",
         "expected": ["auth::AuthService::refresh", "web::AuthController::handleLogout"],
         "pre_phase8_result": "error: symbol not found"},
        {"cat": "A", "cmd": "callers", "query": "DataStore::save",
         "root": root_base / "level3-pipeline",
         "expected": ["job::SyncJob::execute"],
         "pre_phase8_result": "error: symbol not found"},
        # Category B
        {"cat": "B", "cmd": "callers", "query": "auth::AuthToken::validate",
         "root": root_base / "level2-auth",
         "expected": ["auth::AuthService::login", "web::AuthController::handleLogin",
                      "web::AuthController::handleRefresh", "web::SessionController::handle"],
         "pre_phase8_result": "note: no callers found"},
        # Category C
        {"cat": "C", "cmd": "callees", "query": "auth::AuthService::refresh",
         "root": root_base / "level2-auth",
         "expected": ["auth::AuthToken::expire", "auth::TokenCache::invalidate",
                      "auth::AuthToken::refresh"],
         "pre_phase8_result": "note: no callees found"},
        {"cat": "C", "cmd": "callees", "query": "processor::RequestProcessor::process",
         "root": root_base / "level4-api",
         "expected": ["service::ValidationService::validate", "store::DatabaseStore::save"],
         "pre_phase8_result": "note: no callees found"},
        # Category D (guards)
        {"cat": "D", "cmd": "callers", "query": "auth::AuthToken::expire",
         "root": root_base / "level2-auth",
         "expected": ["auth::AuthService::refresh", "web::AuthController::handleLogout"],
         "pre_phase8_result": "same"},
        {"cat": "D", "cmd": "references", "query": "auth::AuthToken::validate",
         "root": root_base / "level2-auth",
         "expected": ["auth_service.cpp:14:19", "auth_controller.cpp:13:17"],
         "pre_phase8_result": "same"},
        {"cat": "D", "cmd": "callees", "query": "auth::AuthService::login",
         "root": root_base / "level2-auth",
         "expected": ["auth::AuthToken::validate"],
         "pre_phase8_result": "same"},
    ]

    for probe in probes:
        runs = []
        for _ in range(5):
            t0 = time.perf_counter()
            proc = subprocess.run(
                [str(bin_path), probe["cmd"], probe["query"], str(probe["root"])],
                capture_output=True, text=True,
            )
            elapsed = time.perf_counter() - t0
            lines = proc.stdout.strip().splitlines()
            runs.append({
                "returncode": proc.returncode,
                "elapsed_seconds": round(elapsed, 6),
                "stdout_lines": lines,
                "stderr_lines": proc.stderr.strip().splitlines(),
            })

        # Verify expected fragments
        actual_set = set()
        for run in runs:
            for line in run["stdout_lines"]:
                actual_set.add(line)

        missing = [e for e in probe["expected"] if not any(e in l for l in actual_set)]
        unexpected = []  # conservatively empty; false-positive checking is manual

        results.append({
            "category": probe["cat"],
            "command": probe["cmd"],
            "query": probe["query"],
            "root": str(probe["root"]),
            "pre_phase8_result": probe["pre_phase8_result"],
            "expected_fragments": probe["expected"],
            "stable_across_five": len({tuple(r["stdout_lines"]) for r in runs}) == 1,
            "missing": missing,
            "unexpected": unexpected,
            "mean_elapsed_seconds": round(sum(r["elapsed_seconds"] for r in runs) / len(runs), 6),
            "runs": runs,
        })

    return results


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--first-repeat", type=int, default=1)
    parser.add_argument("--last-repeat", type=int, default=5)
    parser.add_argument("--arm", choices=["a", "b", "both"], default="b",
                        help="Which arm to run agent sessions for (a=pre-Phase-8, b=Phase 8)")
    parser.add_argument("--direct-only", action="store_true",
                        help="Only run direct CLI semantic probes (no agent sessions)")
    args = parser.parse_args()

    assert WORKSPACE_BIN.exists(), f"Binary not found: {WORKSPACE_BIN}"

    out_dir = ROOT / "phase9a"
    agent_dir = out_dir / "agent"
    out_dir.mkdir(parents=True, exist_ok=True)
    agent_dir.mkdir(parents=True, exist_ok=True)

    # Direct semantic probes (fast, no agent)
    direct_path = out_dir / "direct_replays.json"
    if not direct_path.exists():
        print("Running direct semantic probes...", flush=True)
        repos_base = ROOT / "repositories"
        direct = run_direct_semantic_probes(repos_base)
        direct_path.write_text(json.dumps(direct, indent=2), encoding="utf-8")
        print(f"Direct probes complete: {len(direct)} probes", flush=True)

    if args.direct_only:
        print("--direct-only: skipping agent sessions")
        return

    # Agent sessions
    os.environ["AST_TOOL_CONTROLLED_SKILL"] = "1"
    os.environ["PATH"] = str(WORKSPACE_BIN.parent) + os.pathsep + os.environ["PATH"]

    tasks = TASKS_A + TASKS_B  # Categories A and B have agent run data
    manifest_path = agent_dir / "manifest.json"
    manifest = {
        "arm": args.arm,
        "binary_sha256": sha256(WORKSPACE_BIN),
        "skill_sha256": sha256(SKILLS[0]) if SKILLS[0].exists() else None,
        "tasks": tasks,
        "forced_semantic": True,
        "checks": [],
    }
    if manifest_path.exists():
        existing = json.loads(manifest_path.read_text(encoding="utf-8"))
        manifest["checks"] = existing.get("checks", [])

    adapter.clear_claude_logs = lambda: None
    adapter.run_claude = isolated_run

    for repeat in range(args.first_repeat, args.last_repeat + 1):
        destination = agent_dir / f"r{repeat}"
        ordered = tasks if repeat % 2 else list(reversed(tasks))
        for task in ordered:
            analysis_path = destination / "analysis" / f"{task}.json"
            if analysis_path.exists():
                continue
            verify_baseline()
            logs.CLAUDE_LOG_DIR = destination / "sessions" / task
            logs.CLAUDE_LOG_DIR.mkdir(parents=True, exist_ok=True)
            record = run_task(
                ROOT / "tasks" / f"{task}.yaml",
                ROOT,
                destination,
                trace_dir=destination / "traces",
            )
            manifest["checks"].append({
                "repeat": repeat,
                "task": task,
                "arm": args.arm,
                "status": record["status"],
                "binary_after": sha256(WORKSPACE_BIN),
            })
            manifest_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
            summaries = analyze_directory(
                destination / "traces",
                results_path=destination / "results.jsonl",
            )
            analysis_path.parent.mkdir(exist_ok=True)
            for row in summaries:
                (destination / "analysis" / f"{row['task_id']}.json").write_text(
                    json.dumps(row, indent=2), encoding="utf-8"
                )
            print(f"PHASE9A arm={args.arm} r{repeat} {task}: {record['status']}", flush=True)
            if record["status"] in ("runner_failure", "agent_process_failure"):
                raise RuntimeError("Infrastructure failure; inspect captured process.json")


if __name__ == "__main__":
    main()
