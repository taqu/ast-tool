"""Phase 9b.1 — Fresh Controlled Pre-Phase-8 vs Phase-8 Confirmation.

Runs a fresh, symmetric, interleaved comparison between:
    Arm A: pre-Phase-8 baseline   (git ccc1fbb650bf058aa11602134d4e4fa1795cb98e)
    Arm B: accepted Phase 8a+8b+8c implementation (current HEAD)

under identical forced semantic-routing conditions (AST_TOOL_CONTROLLED_SKILL=1),
using the same accepted Phase 7d semantic-analysis Skill for both arms.

Usage:
    python phase9b1_probe.py [--direct-only] [--agent-only]
                              [--tasks task1,task2,...] [--repeats-affected N]
                              [--repeats-guard N] [--resume]

Binaries must already be built and staged at:
    evaluation/binaries/arm_a/ast-tool.exe
    evaluation/binaries/arm_b/ast-tool.exe
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))

import logs
import agents.claude_code as adapter
from runner import run_task
from trace_analyzer import analyze_directory

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parent
WORKSPACE_BIN = REPO / "bin" / "ast-tool.exe"

ARM_BIN = {
    "A": ROOT / "binaries" / "arm_a" / "ast-tool.exe",
    "B": ROOT / "binaries" / "arm_b" / "ast-tool.exe",
}
ARM_REV = {
    "A": "ccc1fbb650bf058aa11602134d4e4fa1795cb98e",
    "B": "3095337a5ff04c7de723c3a0853682c9cefdf90c",
}

SKILLS = [
    REPO / "skills" / "semantic-analysis" / "SKILL.md",
    Path.home() / ".claude" / "skills" / "semantic-analysis" / "SKILL.md",
]

# Cohort ---------------------------------------------------------------
# Determined from fresh direct-probe reconnaissance (see phase9b1/direct_probes.json):
# level1-002 (references InventoryService::save) FAILS on Arm A even with the
# fully-qualified FQN -- it is Phase-8b-affected, not an unaffected guard, so it
# was moved from Group D into Group B as bonus evidence. level1-003 (find/search
# only, no relationship command) was confirmed arm-invariant and substituted in.
#
# Group A — Phase 8a target resolution (partial FQN)
TASKS_A = ["level2-008", "level3-008"]
# Group B — Phase 8b receiver-type member relationships
TASKS_B = ["level2-004", "level4-006", "level1-002"]
# Group C — Phase 8c body-identity callees (declaration-only -> out-of-line body)
TASKS_C = ["level2-005"]
# Group D — Unaffected semantic guards (confirmed arm-invariant by direct probe)
TASKS_D = ["level1-001", "level1-003", "level2-001", "level3-001"]

AFFECTED_TASKS = TASKS_A + TASKS_B + TASKS_C
GUARD_TASKS = TASKS_D
ALL_TASKS = AFFECTED_TASKS + GUARD_TASKS

TASK_GROUP = {t: "A" for t in TASKS_A}
TASK_GROUP.update({t: "B" for t in TASKS_B})
TASK_GROUP.update({t: "C" for t in TASKS_C})
TASK_GROUP.update({t: "D" for t in TASKS_D})

DEFAULT_REPEATS_AFFECTED = 5
DEFAULT_REPEATS_GUARD = 3

ORIGINAL_RUN = adapter.run_claude


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def isolated_run(prompt: str, cwd: Path, timeout: int):
    import re as _re
    project = (Path.home() / ".claude/projects") / _re.sub(r"[^a-zA-Z0-9]", "-", str(cwd))
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


def install_arm(arm: str, frozen: dict) -> None:
    shutil.copy2(ARM_BIN[arm], WORKSPACE_BIN)
    actual = sha256(WORKSPACE_BIN)
    expected = frozen["binary_sha256"][arm]
    if actual != expected:
        raise AssertionError(
            f"Binary hash mismatch installing arm {arm}: {actual} != {expected}"
        )


def verify_skills_unchanged(frozen: dict) -> None:
    for path in SKILLS:
        if not path.exists():
            raise AssertionError(f"Skill file missing: {path}")
        actual = sha256(path)
        if actual != frozen["skill_sha256"]:
            raise AssertionError(f"Skill hash drifted at {path}: {actual}")


def check_routing(trace_path: Path) -> dict:
    """Verify: Skill(semantic-analysis) invoked exactly once, as the first tool action."""
    skill_positions = []
    first_tool = None
    seq = 0
    with trace_path.open("r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            ev = json.loads(line)
            if ev.get("event") != "tool_call":
                continue
            seq += 1
            if first_tool is None:
                first_tool = ev.get("tool")
            if ev.get("tool") == "Skill":
                inp = ev.get("input") or {}
                skill_name = inp.get("skill") if isinstance(inp, dict) else None
                skill_positions.append({"sequence": seq, "skill": skill_name})
    ok_count = len(skill_positions) == 1
    ok_first = first_tool == "Skill"
    ok_name = bool(skill_positions) and skill_positions[0].get("skill") == "semantic-analysis"
    return {
        "skill_invocations": skill_positions,
        "first_tool": first_tool,
        "routing_ok": ok_count and ok_first and ok_name,
    }


def build_frozen_manifest() -> dict:
    frozen = {
        "binary_sha256": {arm: sha256(ARM_BIN[arm]) for arm in ("A", "B")},
        "binary_git_revision": ARM_REV,
        "skill_sha256": sha256(SKILLS[0]),
        "skill_paths": [str(p) for p in SKILLS],
        "tasks": ALL_TASKS,
        "task_group": TASK_GROUP,
        "forced_semantic": True,
    }
    for path in SKILLS:
        assert sha256(path) == frozen["skill_sha256"], f"Skill mismatch at {path}"
    return frozen


def run_direct_probes(out_path: Path) -> None:
    if out_path.exists():
        print("Direct probes already collected, skipping.")
        return

    repos = ROOT / "repositories"
    probes = [
        {"group": "A", "cmd": "callers", "query": "AuthToken::expire",
         "root": repos / "level2-auth",
         "expected": ["auth::AuthService::refresh", "web::AuthController::handleLogout"]},
        {"group": "A", "cmd": "callers", "query": "DataStore::save",
         "root": repos / "level3-pipeline",
         "expected": ["job::SyncJob::execute"]},
        {"group": "B", "cmd": "callers", "query": "auth::AuthToken::validate",
         "root": repos / "level2-auth",
         "expected": ["auth::AuthService::login", "web::AuthController::handleLogin",
                      "web::AuthController::handleRefresh", "web::SessionController::handle"]},
        {"group": "C", "cmd": "callees", "query": "auth::AuthService::refresh",
         "root": repos / "level2-auth",
         "expected": ["auth::AuthToken::expire", "auth::TokenCache::invalidate",
                      "auth::AuthToken::refresh"]},
        # Bonus Phase 8b evidence: receiver-type fix generalizes beyond `callers`
        # to `references` and `callees` too (discovered during cohort recon).
        {"group": "B", "cmd": "references", "query": "auth::AuthToken::validate",
         "root": repos / "level2-auth",
         "expected": ["auth_service.cpp:14:19", "auth_controller.cpp:13:17"]},
        {"group": "B", "cmd": "callees", "query": "auth::AuthService::login",
         "root": repos / "level2-auth",
         "expected": ["auth::AuthToken::validate"]},
        {"group": "B", "cmd": "references", "query": "store::InventoryService::save",
         "root": repos / "level1-store",
         "expected": ["main.cpp:11:9", "main.cpp:12:9", "order_service.cpp:11:23"]},
        # Group D — confirmed arm-invariant guards (exact FQN / structural, no
        # receiver-type relationship resolution involved).
        {"group": "D", "cmd": "callers", "query": "auth::AuthToken::expire",
         "root": repos / "level2-auth",
         "expected": ["auth::AuthService::refresh", "web::AuthController::handleLogout"]},
        {"group": "D", "cmd": "callers", "query": "service::CheckoutService::process",
         "root": repos / "level3-order",
         "expected": ["web::MobileCheckoutHandler::handle", "web::WebCheckoutHandler::handle"]},
        {"group": "D", "cmd": "search", "query": "--name update",
         "root": repos / "level2-auth",
         "expected": ["auth::AuthService::update", "auth::TokenCache::update",
                      "auth::UserRepository::update", "session::SessionManager::update"]},
    ]

    results = []
    for probe in probes:
        for arm in ("A", "B"):
            bin_path = ARM_BIN[arm]
            runs = []
            query_args = probe["query"].split()
            for _ in range(5):
                t0 = time.perf_counter()
                proc = subprocess.run(
                    [str(bin_path), probe["cmd"], *query_args, str(probe["root"])],
                    capture_output=True, text=True,
                )
                elapsed = time.perf_counter() - t0
                runs.append({
                    "returncode": proc.returncode,
                    "elapsed_seconds": round(elapsed, 6),
                    "stdout_lines": proc.stdout.strip().splitlines(),
                    "stderr_lines": proc.stderr.strip().splitlines(),
                })
            actual_set = {line for r in runs for line in r["stdout_lines"]}
            missing = [e for e in probe["expected"] if not any(e in l for l in actual_set)]
            results.append({
                "group": probe["group"],
                "command": probe["cmd"],
                "query": probe["query"],
                "root": str(probe["root"]),
                "arm": arm,
                "binary_revision": ARM_REV[arm],
                "expected_fragments": probe["expected"],
                "stable_across_five": len({tuple(r["stdout_lines"]) for r in runs}) == 1,
                "missing": missing,
                "unexpected": [],
                "mean_elapsed_seconds": round(sum(r["elapsed_seconds"] for r in runs) / len(runs), 6),
                "runs": runs,
            })

    out_path.write_text(json.dumps(results, indent=2), encoding="utf-8")
    print(f"Direct probes complete: {len(results)} probe x arm combinations")


def repeats_for(task: str, repeats_affected: int, repeats_guard: int) -> int:
    return repeats_guard if task in GUARD_TASKS else repeats_affected


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--direct-only", action="store_true")
    parser.add_argument("--agent-only", action="store_true")
    parser.add_argument("--tasks", type=str, default=None,
                         help="Comma-separated subset of task ids to run")
    parser.add_argument("--repeats-affected", type=int, default=DEFAULT_REPEATS_AFFECTED)
    parser.add_argument("--repeats-guard", type=int, default=DEFAULT_REPEATS_GUARD)
    args = parser.parse_args()

    for arm in ("A", "B"):
        assert ARM_BIN[arm].exists(), f"Missing binary for arm {arm}: {ARM_BIN[arm]}"

    out_dir = ROOT / "phase9b1"
    out_dir.mkdir(parents=True, exist_ok=True)
    agent_dir = out_dir / "agent"
    agent_dir.mkdir(parents=True, exist_ok=True)

    manifest_path = out_dir / "manifest.json"
    if manifest_path.exists():
        frozen = json.loads(manifest_path.read_text(encoding="utf-8"))
        # re-verify nothing drifted since manifest was written
        for arm in ("A", "B"):
            actual = sha256(ARM_BIN[arm])
            assert actual == frozen["binary_sha256"][arm], f"Arm {arm} binary drifted!"
        verify_skills_unchanged(frozen)
    else:
        frozen = build_frozen_manifest()
        frozen["checks"] = []
        manifest_path.write_text(json.dumps(frozen, indent=2), encoding="utf-8")

    print("Frozen inputs:")
    print(json.dumps({k: v for k, v in frozen.items() if k != "checks"}, indent=2))

    if not args.agent_only:
        run_direct_probes(out_dir / "direct_probes.json")

    if args.direct_only:
        return

    tasks = ALL_TASKS
    if args.tasks:
        wanted = set(args.tasks.split(","))
        tasks = [t for t in ALL_TASKS if t in wanted]

    os.environ["AST_TOOL_CONTROLLED_SKILL"] = "1"
    os.environ["PATH"] = str(WORKSPACE_BIN.parent) + os.pathsep + os.environ["PATH"]

    adapter.clear_claude_logs = lambda: None
    adapter.run_claude = isolated_run

    checks = frozen.setdefault("checks", [])
    done = {(c["task"], c["repeat"], c["arm"]) for c in checks if c.get("status") not in
            (None, "runner_failure", "agent_process_failure")}

    task_index = {t: i for i, t in enumerate(ALL_TASKS)}

    for task in tasks:
        n_repeats = repeats_for(task, args.repeats_affected, args.repeats_guard)
        for repeat in range(1, n_repeats + 1):
            order = ["A", "B"] if (task_index[task] + repeat) % 2 == 0 else ["B", "A"]
            for position, arm in enumerate(order, start=1):
                if (task, repeat, arm) in done:
                    continue

                install_arm(arm, frozen)
                verify_skills_unchanged(frozen)

                destination = agent_dir / task / f"r{repeat}" / f"arm_{arm}"
                logs.CLAUDE_LOG_DIR = destination / "sessions"
                logs.CLAUDE_LOG_DIR.mkdir(parents=True, exist_ok=True)

                record = run_task(
                    ROOT / "tasks" / f"{task}.yaml",
                    ROOT,
                    destination,
                    trace_dir=destination / "traces",
                )

                binary_after = sha256(WORKSPACE_BIN)

                routing = {}
                trace_path = destination / "traces" / f"{task}.jsonl"
                if trace_path.exists():
                    routing = check_routing(trace_path)

                check_entry = {
                    "task": task,
                    "group": TASK_GROUP[task],
                    "repeat": repeat,
                    "arm": arm,
                    "order_position": position,
                    "status": record["status"],
                    "success": record["success"],
                    "binary_before": frozen["binary_sha256"][arm],
                    "binary_after": binary_after,
                    "binary_stable": binary_after == frozen["binary_sha256"][arm],
                    "routing": routing,
                }
                checks.append(check_entry)
                manifest_path.write_text(json.dumps(frozen, indent=2), encoding="utf-8")

                if trace_path.exists():
                    summaries = analyze_directory(
                        destination / "traces",
                        results_path=destination / "results.jsonl",
                    )
                    analysis_dir = destination / "analysis"
                    analysis_dir.mkdir(exist_ok=True)
                    for row in summaries:
                        (analysis_dir / f"{row['task_id']}.json").write_text(
                            json.dumps(row, indent=2), encoding="utf-8"
                        )

                routing_flag = routing.get("routing_ok")
                print(
                    f"PHASE9B1 task={task} group={TASK_GROUP[task]} r{repeat} "
                    f"arm={arm} pos={position}: status={record['status']} "
                    f"routing_ok={routing_flag} binary_stable={check_entry['binary_stable']}",
                    flush=True,
                )

                if record["status"] in ("runner_failure", "agent_process_failure"):
                    raise RuntimeError(
                        f"Infrastructure failure on {task} r{repeat} arm={arm}; "
                        "inspect captured process.json"
                    )
                if not check_entry["binary_stable"]:
                    raise RuntimeError(f"Binary mutated during run: {task} r{repeat} arm={arm}")


if __name__ == "__main__":
    main()
