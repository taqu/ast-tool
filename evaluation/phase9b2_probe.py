"""Phase 9b.2 -- Final Normal-Routing Agent-Level Evaluation.

Runs the full 41-task suite under NORMAL routing (no forced Skill
invocation) for both Arm A (pre-Phase-8, ccc1fbb) and Arm B (accepted
Phase 8a+8b+8c, current HEAD), interleaved, one run per arm per task
(stage 1). A selected subset of tasks can then be repeated (stage 2)
via --tasks/--repeats.

Usage:
    python phase9b2_probe.py --stage1
    python phase9b2_probe.py --stage2 --tasks level2-008,level3-008,... --repeats 5
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import sys
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

ALL_TASKS = [p.stem for p in sorted((ROOT / "tasks").glob("*.yaml"))]

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
        raise AssertionError(f"Binary hash mismatch installing arm {arm}: {actual} != {expected}")


def verify_skills_unchanged(frozen: dict) -> None:
    for path in SKILLS:
        if not path.exists():
            raise AssertionError(f"Skill file missing: {path}")
        actual = sha256(path)
        if actual != frozen["skill_sha256"]:
            raise AssertionError(f"Skill hash drifted at {path}: {actual}")


def classify_routing(trace_path: Path) -> dict:
    """Classify normal-routing behavior: A-E per ast-tool.md Skill Invocation Measurement."""
    skill_calls = []  # list of {sequence, skill}
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
                first_ast = bool(ev.get("ast_tool"))
            if ev.get("tool") == "Skill":
                inp = ev.get("input") or {}
                name = inp.get("skill") if isinstance(inp, dict) else None
                skill_calls.append({"sequence": seq, "skill": name})

    semantic_calls = [c for c in skill_calls if c["skill"] == "semantic-analysis"]
    other_calls = [c for c in skill_calls if c["skill"] != "semantic-analysis"]

    if semantic_calls and semantic_calls[0]["sequence"] == 1:
        category = "A"  # semantic-analysis first
    elif other_calls and other_calls[0]["sequence"] == 1:
        category = "B"  # another Skill first
    elif first_tool in ("Agent", "Task"):
        category = "C"  # subagent exploration first
    elif first_tool == "Bash" and first_ast:
        category = "D"  # direct AST tool first
    else:
        category = "E"  # Grep/Glob/Read/manual first

    return {
        "category": category,
        "first_tool": first_tool,
        "skill_calls": skill_calls,
        "semantic_analysis_invoked": bool(semantic_calls),
        "semantic_analysis_count": len(semantic_calls),
        "semantic_analysis_first_position": semantic_calls[0]["sequence"] if semantic_calls else None,
        "other_skill_invoked": bool(other_calls),
    }


def build_frozen_manifest() -> dict:
    frozen = {
        "phase": "9b2",
        "binary_sha256": {arm: sha256(ARM_BIN[arm]) for arm in ("A", "B")},
        "binary_git_revision": ARM_REV,
        "skill_sha256": sha256(SKILLS[0]),
        "skill_paths": [str(p) for p in SKILLS],
        "all_tasks": ALL_TASKS,
        "forced_semantic": False,
    }
    for path in SKILLS:
        assert sha256(path) == frozen["skill_sha256"], f"Skill mismatch at {path}"
    return frozen


def run_one(task: str, arm: str, repeat: int, frozen: dict, checks: list, manifest_path: Path) -> dict:
    install_arm(arm, frozen)
    verify_skills_unchanged(frozen)

    destination = ROOT / "phase9b2" / "agent" / task / f"r{repeat}" / f"arm_{arm}"
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
        routing = classify_routing(trace_path)

    check_entry = {
        "task": task, "repeat": repeat, "arm": arm,
        "status": record["status"], "success": record["success"],
        "binary_before": frozen["binary_sha256"][arm],
        "binary_after": binary_after,
        "binary_stable": binary_after == frozen["binary_sha256"][arm],
        "routing": routing,
    }
    checks.append(check_entry)
    manifest_path.write_text(json.dumps(frozen, indent=2), encoding="utf-8")

    if trace_path.exists():
        summaries = analyze_directory(destination / "traces", results_path=destination / "results.jsonl")
        analysis_dir = destination / "analysis"
        analysis_dir.mkdir(exist_ok=True)
        for row in summaries:
            (analysis_dir / f"{row['task_id']}.json").write_text(json.dumps(row, indent=2), encoding="utf-8")

    print(
        f"PHASE9B2 task={task} r{repeat} arm={arm}: status={record['status']} "
        f"routing_cat={routing.get('category')} sem_first_pos={routing.get('semantic_analysis_first_position')} "
        f"binary_stable={check_entry['binary_stable']}",
        flush=True,
    )

    if record["status"] in ("runner_failure", "agent_process_failure"):
        raise RuntimeError(f"Infrastructure failure on {task} r{repeat} arm={arm}; inspect process.json")
    if not check_entry["binary_stable"]:
        raise RuntimeError(f"Binary mutated during run: {task} r{repeat} arm={arm}")

    return check_entry


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--stage1", action="store_true", help="Run the full 41-task suite once per arm")
    parser.add_argument("--stage2", action="store_true", help="Run additional repeats for --tasks")
    parser.add_argument("--tasks", type=str, default=None, help="Comma-separated task ids (stage2)")
    parser.add_argument("--repeats", type=int, default=5, help="Total repeats per arm to reach (stage2)")
    args = parser.parse_args()

    for arm in ("A", "B"):
        assert ARM_BIN[arm].exists(), f"Missing binary for arm {arm}: {ARM_BIN[arm]}"

    out_dir = ROOT / "phase9b2"
    out_dir.mkdir(parents=True, exist_ok=True)
    manifest_path = out_dir / "manifest.json"

    if manifest_path.exists():
        frozen = json.loads(manifest_path.read_text(encoding="utf-8"))
        for arm in ("A", "B"):
            actual = sha256(ARM_BIN[arm])
            assert actual == frozen["binary_sha256"][arm], f"Arm {arm} binary drifted!"
        verify_skills_unchanged(frozen)
    else:
        frozen = build_frozen_manifest()
        frozen["checks"] = []
        manifest_path.write_text(json.dumps(frozen, indent=2), encoding="utf-8")

    print("Frozen inputs:")
    print(json.dumps({k: v for k, v in frozen.items() if k != "checks"}, indent=2)[:2000])

    # Normal routing: do NOT set AST_TOOL_CONTROLLED_SKILL
    os.environ.pop("AST_TOOL_CONTROLLED_SKILL", None)
    os.environ["PATH"] = str(WORKSPACE_BIN.parent) + os.pathsep + os.environ["PATH"]

    adapter.clear_claude_logs = lambda: None
    adapter.run_claude = isolated_run

    checks = frozen.setdefault("checks", [])
    done = {(c["task"], c["repeat"], c["arm"]) for c in checks
            if c.get("status") not in (None, "runner_failure", "agent_process_failure")}

    if args.stage1:
        task_index = {t: i for i, t in enumerate(ALL_TASKS)}
        for task in ALL_TASKS:
            order = ["A", "B"] if task_index[task] % 2 == 0 else ["B", "A"]
            for arm in order:
                if (task, 1, arm) in done:
                    continue
                run_one(task, arm, 1, frozen, checks, manifest_path)

    if args.stage2:
        assert args.tasks, "--stage2 requires --tasks"
        tasks = args.tasks.split(",")
        task_index = {t: i for i, t in enumerate(tasks)}
        for repeat in range(2, args.repeats + 1):
            order_tasks = tasks if repeat % 2 == 0 else list(reversed(tasks))
            for task in order_tasks:
                order = ["A", "B"] if (task_index[task] + repeat) % 2 == 0 else ["B", "A"]
                for arm in order:
                    if (task, repeat, arm) in done:
                        continue
                    run_one(task, arm, repeat, frozen, checks, manifest_path)


if __name__ == "__main__":
    main()
