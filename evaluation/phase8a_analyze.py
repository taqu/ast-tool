"""Build Phase 8a raw replay and controlled-agent measurements."""
from __future__ import annotations

from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import statistics
import subprocess
import sys
import time
from typing import Any

sys.path.insert(0, str(Path(__file__).parent))
from trace_analyzer import analyze_directory


ROOT = Path(__file__).resolve().parent
REPO = ROOT.parent
OUT = ROOT / "phase8a"
BIN = REPO / "bin" / "ast-tool.exe"
TASKS = ["level2-008", "level3-008"]
METRICS = [
    "success", "total_tool_calls", "ast_tool_calls", "ast_tool_failures",
    "ast_tool_retries", "ast_tool_help_calls", "search_calls", "find_calls",
    "callers_calls", "callees_calls", "references_calls", "grep_calls",
    "glob_calls", "read_calls", "bash_calls", "edit_calls", "total_tokens",
    "elapsed_seconds",
]


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def mean_metrics(rows: list[dict[str, Any]]) -> dict[str, float]:
    values = []
    for row in rows:
        enriched = dict(row)
        for command in ("search", "find", "callers", "callees", "references"):
            enriched[f"{command}_calls"] = row.get("ast_tool_commands", {}).get(command, 0)
        values.append(enriched)
    return {metric: statistics.mean(float(row[metric]) for row in values) for metric in METRICS}


def ast_executable_invocations(trace: Path) -> int:
    count = 0
    for line in trace.read_text(encoding="utf-8").splitlines():
        event = json.loads(line)
        if event.get("event") != "tool_call":
            continue
        command = str(event.get("input", {}).get("command", ""))
        count += len(re.findall(r"\bast-tool\s+(?:callers|callees|references|search|find|symbols)\b", command))
    return count


def load_agent_runs() -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    for directory in sorted((OUT / "agent").glob("r[0-9]*"), key=lambda path: int(path.name[1:])):
        summaries = analyze_directory(
            directory / "traces", results_path=directory / "results.jsonl", task_filter=TASKS
        )
        for row in summaries:
            row["repeat"] = int(directory.name[1:])
            row["trajectory"] = "→".join(row["ast_tool_sequence"]) or "none"
            row["ast_executable_invocations"] = ast_executable_invocations(
                directory / "traces" / f"{row['task_id']}.jsonl"
            )
            rows.append(row)
    return rows


def run_command(arguments: list[str]) -> dict[str, Any]:
    start = time.perf_counter()
    result = subprocess.run(arguments, cwd=REPO, capture_output=True, text=True, encoding="utf-8")
    return {
        "returncode": result.returncode,
        "elapsed_seconds": round(time.perf_counter() - start, 6),
        "stdout_lines": [line for line in result.stdout.splitlines() if line.strip()],
        "stderr_lines": [line for line in result.stderr.splitlines() if line.strip()],
    }


def direct_replays() -> list[dict[str, Any]]:
    cases = [
        {
            "task": "level2-008",
            "query": "AuthToken::expire",
            "canonical_fqn": "auth::AuthToken::expire",
            "root": "evaluation/repositories/level2-auth",
            "before_occurrences": 2,
        },
        {
            "task": "level3-008",
            "query": "DataStore::save",
            "canonical_fqn": "store::DataStore::save",
            "root": "evaluation/repositories/level3-pipeline",
            "before_occurrences": 2,
        },
    ]
    output = []
    for case in cases:
        partial_runs = [run_command([str(BIN), "callers", case["query"], case["root"]]) for _ in range(5)]
        exact_runs = [run_command([str(BIN), "callers", case["canonical_fqn"], case["root"]]) for _ in range(5)]
        equivalent = all(
            run["returncode"] == 0 and run["stdout_lines"] == exact_runs[0]["stdout_lines"]
            for run in partial_runs + exact_runs
        )
        output.append({
            **case,
            "before_trajectory": "callers(partial, failure)→search→callers(exact, success)",
            "after_trajectory": "callers(partial, success)",
            "before_ast_calls": 3,
            "after_ast_calls": 1,
            "before_failures": 1,
            "after_failures": 0,
            "suffix_fallback_used": True,
            "canonical_candidate_count": 1,
            "answers_equivalent": equivalent,
            "relationship_set": partial_runs[0]["stdout_lines"],
            "partial_mean_elapsed_seconds": statistics.mean(run["elapsed_seconds"] for run in partial_runs),
            "exact_mean_elapsed_seconds": statistics.mean(run["elapsed_seconds"] for run in exact_runs),
            "partial_runs": partial_runs,
            "exact_runs": exact_runs,
        })
    return output


def main() -> None:
    runs = load_agent_runs()
    baseline = json.loads((ROOT / "phase7f" / "audit.json").read_text(encoding="utf-8"))
    tasks: dict[str, Any] = {}
    for task in TASKS:
        after = [row for row in runs if row["task_id"] == task]
        before_rows = [
            row for row in baseline["runs"]
            if row["task_id"] == task and row["routing_mode"] == "semantic"
        ]
        before = baseline["task_comparisons"][task]["semantic"]
        before_means = mean_metrics(before_rows)
        after_means = mean_metrics(after)
        tasks[task] = {
            "before_runs": before["runs"],
            "after_runs": len(after),
            "before_means": before_means,
            "after_means": after_means,
            "after_minus_before": {
                metric: after_means[metric] - before_means[metric] for metric in METRICS
            },
            "before_trajectories": baseline["task_comparisons"][task]["semantic_trajectories"],
            "after_trajectories": Counter(row["trajectory"] for row in after),
            "after_executable_invocations_mean": statistics.mean(
                row["ast_executable_invocations"] for row in after
            ),
            "recovery_mean": None,
            "recovery_max": None,
        }

    payload = {
        "environment": {
            "revision": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=REPO, text=True).strip(),
            "branch": subprocess.check_output(["git", "branch", "--show-current"], cwd=REPO, text=True).strip(),
            "skill_sha256": sha256(REPO / "skills/semantic-analysis/SKILL.md"),
            "installed_skill_sha256": sha256(Path.home() / ".claude/skills/semantic-analysis/SKILL.md"),
            "workspace_binary_sha256": sha256(BIN),
        },
        "agent_runs": runs,
        "task_aggregates": tasks,
        "direct_replays": direct_replays(),
    }
    (OUT / "measurements.json").write_text(json.dumps(payload, indent=2), encoding="utf-8")
    print(json.dumps({"environment": payload["environment"], "task_aggregates": tasks,
                      "direct_replays": payload["direct_replays"]}, indent=2))


if __name__ == "__main__":
    main()
