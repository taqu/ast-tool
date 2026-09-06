"""Run the Phase 8b controlled semantic agent probe against the workspace binary."""
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

sys.path.insert(0, str(Path(__file__).parent))

import logs
import agents.claude_code as adapter
from runner import run_task
from trace_analyzer import analyze_directory


ROOT = Path(__file__).resolve().parent
REPO = ROOT.parent
OUT = ROOT / "phase8b" / "agent"
TASKS = ["level2-004", "level4-006"]
EXPECTED_SKILL = "96b07a6b89ae338f26d45fbfb31dd97d5b9c50efa39922103e8cb3e616807eaf"
SKILLS = [REPO / "skills/semantic-analysis/SKILL.md",
          Path.home() / ".claude/skills/semantic-analysis/SKILL.md"]
GLOBAL_LOGS = Path.home() / ".claude/projects"
WORKSPACE_BIN = REPO / "bin" / "ast-tool.exe"
ORIGINAL_RUN = adapter.run_claude


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify_baseline() -> dict[str, str]:
    result = {str(path): sha256(path) for path in SKILLS}
    assert all(value == EXPECTED_SKILL for value in result.values()), result
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
        "exit_code": result.exit_code, "stderr": result.stderr,
        "stdout": result.stdout, "fresh_logs": len(fresh),
    }, indent=2), encoding="utf-8")
    return result


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--first-repeat", type=int, default=1)
    parser.add_argument("--last-repeat", type=int, default=5)
    args = parser.parse_args()

    assert WORKSPACE_BIN.exists(), WORKSPACE_BIN
    os.environ["AST_TOOL_CONTROLLED_SKILL"] = "1"
    os.environ["PATH"] = str(WORKSPACE_BIN.parent) + os.pathsep + os.environ["PATH"]
    OUT.mkdir(parents=True, exist_ok=True)
    manifest_path = OUT / "manifest.json"
    manifest = {
        "revision": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=REPO, text=True).strip(),
        "tasks": TASKS, "forced_semantic": True,
        "skill_before": verify_baseline(),
        "workspace_binary_sha256": sha256(WORKSPACE_BIN), "checks": [],
    }
    if manifest_path.exists():
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))

    adapter.clear_claude_logs = lambda: None
    adapter.run_claude = isolated_run
    for repeat in range(args.first_repeat, args.last_repeat + 1):
        destination = OUT / f"r{repeat}"
        ordered = TASKS if repeat % 2 else list(reversed(TASKS))
        for task in ordered:
            analysis_path = destination / "analysis" / f"{task}.json"
            if analysis_path.exists():
                continue
            verify_baseline()
            logs.CLAUDE_LOG_DIR = destination / "sessions" / task
            logs.CLAUDE_LOG_DIR.mkdir(parents=True, exist_ok=True)
            record = run_task(ROOT / "tasks" / f"{task}.yaml", ROOT, destination,
                              trace_dir=destination / "traces")
            manifest["checks"].append({
                "repeat": repeat, "task": task, "status": record["status"],
                "skill_after": verify_baseline(), "binary_after": sha256(WORKSPACE_BIN),
            })
            manifest_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
            summaries = analyze_directory(destination / "traces",
                                           results_path=destination / "results.jsonl")
            analysis_path.parent.mkdir(exist_ok=True)
            for row in summaries:
                (destination / "analysis" / f"{row['task_id']}.json").write_text(
                    json.dumps(row, indent=2), encoding="utf-8")
            print(f"PHASE8B {repeat} {task}: {record['status']}", flush=True)
            if record["status"] in ("runner_failure", "agent_process_failure") or not record.get("tools"):
                raise RuntimeError("Infrastructure failure; inspect captured process.json")


if __name__ == "__main__":
    main()
