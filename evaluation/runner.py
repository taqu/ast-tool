import datetime
import json
import subprocess
import sys
from pathlib import Path

import yaml

sys.path.insert(0, str(Path(__file__).parent))

from validator import ValidationResult, validate_task
from agents.base import AgentRunResult
from agents.claude_code import ClaudeCodeRunner
from agents.antigravity import AntigravityRunner


def load_task(yaml_path: Path) -> dict:
    with open(yaml_path, "r", encoding="utf-8") as f:
        return yaml.safe_load(f)


def reset_repository(repo_path: Path) -> None:
    subprocess.run(
        ["git", "reset", "--hard", "HEAD"],
        cwd=str(repo_path), check=True, capture_output=True,
    )
    subprocess.run(
        ["git", "clean", "-fdx"],
        cwd=str(repo_path), check=True, capture_output=True,
    )


def collect_git_diff(repo_path: Path) -> dict:
    names = subprocess.run(
        ["git", "diff", "--name-only"],
        cwd=str(repo_path), capture_output=True, text=True,
    )
    changed_files = [f for f in names.stdout.splitlines() if f.strip()]

    diff = subprocess.run(
        ["git", "diff"],
        cwd=str(repo_path), capture_output=True, text=True,
    )
    return {"changed_files": changed_files, "diff": diff.stdout}


def _git_revision(repo_path: Path) -> str:
    r = subprocess.run(
        ["git", "rev-parse", "HEAD"],
        cwd=str(repo_path), capture_output=True, text=True,
    )
    return r.stdout.strip()


def _determine_status(exec_result: AgentRunResult, val_result: ValidationResult) -> str:
    if exec_result.exit_code == -2:
        return "runner_failure"
    if exec_result.timed_out:
        return "agent_timeout"
    if exec_result.exit_code != 0:
        return "agent_process_failure"
    if not val_result.success:
        return "validation_failure"
    return "success"


def _write_result(record: dict, results_dir: Path) -> None:
    results_dir.mkdir(parents=True, exist_ok=True)
    out_path = results_dir / "results.jsonl"
    with open(out_path, "a", encoding="utf-8") as f:
        f.write(json.dumps(record) + "\n")
    print(f"[runner] Result appended to {out_path}")


def run_task(task_yaml: Path, base_dir: Path, results_dir: Path, agent_name: str = "claude") -> dict:
    task = load_task(task_yaml)
    task_id = task["id"]
    repo_path = (base_dir / task["repository"]).resolve()
    timeout = task.get("timeout", 300)

    print(f"\n[runner] ── Task: {task_id} ({agent_name}) ──")
    print(f"[runner] Repository: {repo_path}")

    if not repo_path.exists():
        record = {
            "agent": agent_name,
            "task_id": task_id,
            "status": "runner_failure",
            "success": False,
            "timestamp": datetime.datetime.utcnow().isoformat() + "Z",
            "error": f"Repository not found: {repo_path}. Run setup_repos.py first.",
        }
        _write_result(record, results_dir)
        return record

    try:
        reset_repository(repo_path)
    except Exception as e:
        record = {
            "agent": agent_name,
            "task_id": task_id,
            "status": "runner_failure",
            "success": False,
            "timestamp": datetime.datetime.utcnow().isoformat() + "Z",
            "error": f"Repository reset failed: {e}",
        }
        _write_result(record, results_dir)
        return record

    repo_rev = _git_revision(repo_path)

    print(f"[runner] Launching agent '{agent_name}' (timeout={timeout}s) ...")
    if agent_name == "antigravity":
        runner = AntigravityRunner()
    else:
        runner = ClaudeCodeRunner()

    exec_result = runner.run(task["prompt"], repo_path, timeout=timeout)
    print(
        f"[runner] Agent exited: code={exec_result.exit_code}, "
        f"elapsed={exec_result.elapsed_seconds}s, timed_out={exec_result.timed_out}"
    )

    diff_info = collect_git_diff(repo_path)

    print(f"[runner] Running validation ...")
    val_result = validate_task(task, repo_path)
    print(f"[runner] Validation: success={val_result.success}, exit_code={val_result.exit_code}")

    status = _determine_status(exec_result, val_result)

    record: dict = {
        "agent": agent_name,
        "task_id": task_id,
        "status": status,
        "success": status == "success",
        "timestamp": datetime.datetime.utcnow().isoformat() + "Z",
        "repository_revision": repo_rev,
        "elapsed_seconds": exec_result.elapsed_seconds,
        "process_exit_code": exec_result.exit_code,
        "timed_out": exec_result.timed_out,
        "tokens": exec_result.tokens,
        "tools": exec_result.tools,
        "ast_tool": exec_result.ast_tool,
        "workflow": exec_result.workflow,
        "changed_files": diff_info["changed_files"],
        "validation": {
            "success": val_result.success,
            "exit_code": val_result.exit_code,
            "stdout": val_result.stdout[:2000],
            "stderr": val_result.stderr[:2000],
            "expected_files_missing": val_result.expected_files_missing,
        },
    }

    if exec_result.stderr:
        record["agent_stderr"] = exec_result.stderr[:2000]

    _write_result(record, results_dir)

    try:
        reset_repository(repo_path)
    except Exception as e:
        print(f"[runner] Warning: post-task reset failed: {e}")

    return record
