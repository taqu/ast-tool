"""Build the Phase 7f semantic-value and per-AST-call audit datasets."""
from __future__ import annotations

from collections import Counter, defaultdict
from datetime import datetime
import hashlib
import json
from pathlib import Path
import re
import statistics
import subprocess
from typing import Any
import csv

import yaml

from trace_analyzer import analyze_directory, _get_ast_meta, _is_help_invocation


ROOT = Path(__file__).resolve().parent
REPO = ROOT.parent
OUT = ROOT / "phase7f"
COHORT = [
    "level1-006", "level2-001", "level2-004", "level2-005", "level2-006",
    "level2-008", "level3-005", "level3-007", "level3-008", "level4-003",
    "level4-006", "smoke-001",
]
METRICS = [
    "success", "total_tool_calls", "ast_tool_calls", "ast_tool_failures",
    "ast_tool_retries", "ast_tool_help_calls", "grep_calls", "glob_calls",
    "read_calls", "bash_calls", "edit_calls", "total_tokens", "elapsed_seconds",
]


def _events(path: Path) -> list[dict[str, Any]]:
    events = [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines()]
    calls = [event for event in events if event.get("event") == "tool_call"]
    calls.sort(key=lambda event: (event.get("started_at") or "", event.get("sequence", 0)))
    return calls


def _skill_loaded(events: list[dict[str, Any]]) -> bool:
    return any(
        event.get("tool") == "Skill"
        and event.get("input", {}).get("skill", "").split(":")[-1] == "semantic-analysis"
        and event.get("success") is not False
        for event in events
    )


def _duration(event: dict[str, Any]) -> float | None:
    try:
        start = datetime.fromisoformat(event["started_at"].replace("Z", "+00:00"))
        end = datetime.fromisoformat(event["ended_at"].replace("Z", "+00:00"))
        return round((end - start).total_seconds(), 3)
    except (KeyError, TypeError, ValueError):
        return None


def _target(raw: str, command: str) -> str:
    match = re.search(rf"\bast-tool\s+{re.escape(command)}\s+(.*)", raw)
    if not match:
        return ""
    tail = match.group(1).strip()
    quoted = re.findall(r'"([^"]+)"', tail)
    tokens = re.findall(r"(?:[^\s\"]|\"[^\"]*\")+", tail)
    if command == "search":
        for flag in ("--name", "--fqn", "--fqn-regex", "--kind"):
            if flag in tokens:
                index = tokens.index(flag)
                return f"{flag} {tokens[index + 1]}" if index + 1 < len(tokens) else flag
    if command == "find":
        for flag in ("--text", "--type", "--id", "--line"):
            if flag in tokens:
                index = tokens.index(flag)
                value = tokens[index + 1] if index + 1 < len(tokens) else ""
                return f"{flag} {value}".strip()
        return " ".join(tokens[:2])
    return tokens[0] if tokens else (quoted[0] if quoted else "")


def _information(command: str, output: str, success: bool) -> str:
    normalized = output.strip()
    lower = normalized.lower()
    if not success or lower.startswith("error") or "exit code 1" in lower:
        first = next((line.strip() for line in normalized.splitlines() if "error:" in line.lower()), "command failed")
        return first[:240]
    if not normalized:
        return "no output"
    if "no callers found" in lower or "no callees found" in lower or "no references found" in lower:
        return "empty relationship result"
    lines = [line for line in normalized.splitlines() if line.strip()]
    if command == "search":
        return f"{len(lines)} symbol result(s); {lines[0][:180]}"
    if command in ("callers", "callees", "references"):
        return f"{len(lines)} relationship result line(s); {lines[0][:180]}"
    if command == "find":
        return f"{len(lines)} AST node line(s); {lines[0][:180]}"
    return lines[0][:220]


def _search_result_role(command: str, output: str) -> str | None:
    if command != "search":
        return None
    has_header = bool(re.search(r"\.(?:h|hpp):\d+", output, re.IGNORECASE))
    has_source = bool(re.search(r"\.(?:c|cc|cpp|cxx):\d+", output, re.IGNORECASE))
    if has_header and has_source:
        return "declaration-and-definition"
    if has_header:
        return "declaration-only"
    if has_source:
        return "definition-only"
    return "no-role-evidence"


def _next_action(events: list[dict[str, Any]], index: int) -> str:
    if index + 1 >= len(events):
        return "task end"
    event = events[index + 1]
    meta = _get_ast_meta(event)
    if meta:
        return f"ast-tool:{meta.get('command')} {_target(meta.get('raw_command', ''), meta.get('command', ''))}".strip()
    return event.get("tool", "Unknown")


def _classify(
    events: list[dict[str, Any]], index: int, meta: dict[str, Any],
    prior_ast: list[dict[str, Any]],
) -> tuple[str, str, str]:
    command = meta.get("command", "unknown")
    raw = meta.get("raw_command", "")
    target = _target(raw, command)
    output = str(events[index].get("output", ""))
    lower = output.lower()
    success = bool(events[index].get("success", True)) and "exit code 1" not in lower and "error:" not in lower
    next_event = events[index + 1] if index + 1 < len(events) else None
    next_meta = _get_ast_meta(next_event) if next_event else None
    previous = prior_ast[-1] if prior_ast else None
    previous_command = previous["command"] if previous else None
    previous_target = previous["target"] if previous else None
    previous_empty = bool(previous and previous.get("empty"))

    if not success or command in ("top_level", "help") or _is_help_invocation(raw) or command not in ("search", "find", "callers", "callees", "references", "symbols"):
        return "F", "recovery / error-handling overhead", "reasonable attempt; command rejected the input or protocol"
    if command in ("callers", "callees", "references") and (
        "no callers found" in lower or "no callees found" in lower or "no references found" in lower
    ):
        return "C", "relationship retry overhead", "reasonable query, but empty output forced another strategy"
    if command in ("callers", "callees", "references") and previous_command == command and previous_target == target and previous_empty:
        return "C", "relationship retry overhead", "retry after an empty relationship result"
    if command == "search" and previous_command in ("callers", "callees", "references") and previous_empty:
        return "B", "identity / resolution overhead", "reasonable recovery to resolve the relationship target"
    if command == "search" and next_meta and next_meta.get("command") in ("callers", "callees", "references"):
        return "B", "identity / resolution overhead", "reasonable identity step required before a relationship query"
    if command == "search" and ("completed with no output" in lower or not output.strip()):
        return "B", "identity / resolution overhead", "unproductive identity probe; it returned no symbol information"
    if command == "search" and previous_command == "search" and previous_target == target:
        return "B", "identity / resolution overhead", "repeated identity probe"
    if command == "find" and next_event and next_event.get("tool") == "Read":
        return "E", "context-fetch overhead", "reasonable structural location step; editable source still required a Read"
    if command == "find" and any(item["command"] == "find" for item in prior_ast[-2:]):
        return "E", "context-fetch overhead", "reasonable per-file implementation lookup before source retrieval"
    return "A", "necessary semantic query", "provided task-relevant identity, relationship, or structure"


def _load_source(label: str, trace_dir: Path, results: Path) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    summaries = {row["task_id"]: row for row in analyze_directory(trace_dir, results_path=results, task_filter=COHORT)}
    runs: list[dict[str, Any]] = []
    annotations: list[dict[str, Any]] = []
    for task in COHORT:
        path = trace_dir / f"{task}.jsonl"
        if not path.exists() or task not in summaries:
            continue
        events = _events(path)
        loaded = _skill_loaded(events)
        ast_count = sum(_get_ast_meta(event) is not None for event in events)
        mode = "semantic" if loaded else "direct-ast" if ast_count else "manual"
        summary = dict(summaries[task], source=label, routing_mode=mode, skill_invoked=loaded)
        summary["ast_tool_sequence"] = [
            _get_ast_meta(event).get("command") for event in events if _get_ast_meta(event)
        ]
        runs.append(summary)

        prior_ast: list[dict[str, Any]] = []
        for index, event in enumerate(events):
            meta = _get_ast_meta(event)
            if not meta:
                continue
            command = meta.get("command", "unknown")
            output = str(event.get("output", ""))
            empty = any(marker in output.lower() for marker in ("no callers found", "no callees found", "no references found", "no matches"))
            code, category, judgment = _classify(events, index, meta, prior_ast)
            effective_success = bool(event.get("success", True)) and "exit code 1" not in output.lower() and "error:" not in output.lower()
            annotations.append({
                "source": label, "task": task, "routing_mode": mode,
                "ast_index": len(prior_ast) + 1, "command": command,
                "target": _target(meta.get("raw_command", ""), command),
                "raw_command": meta.get("raw_command", ""),
                "success": effective_success,
                "duration_seconds": _duration(event), "output_chars": len(output),
                "search_result_role": _search_result_role(command, output),
                "information_gained": _information(command, output, effective_success),
                "reason_for_call": category, "next_action": _next_action(events, index),
                "classification": code, "classification_label": category,
                "hindsight_assessment": judgment,
            })
            prior_ast.append({"command": command, "target": _target(meta.get("raw_command", ""), command), "empty": empty, "classification": code})
    return runs, annotations


def _phase7e() -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    all_runs: list[dict[str, Any]] = []
    all_annotations: list[dict[str, Any]] = []
    for directory in sorted((ROOT / "phase7e/normal").glob("r[0-9]*"), key=lambda p: int(p.name[1:])):
        runs, annotations = _load_source(
            f"phase7e-{directory.name}", directory / "traces", directory / "results.jsonl"
        )
        all_runs.extend(runs)
        all_annotations.extend(annotations)
    return all_runs, all_annotations


def _stats(rows: list[dict[str, Any]]) -> dict[str, Any] | None:
    if not rows:
        return None
    distances = [distance for row in rows for distance in row.get("ast_tool_recovery_distances", [])]
    return {
        "runs": len(rows),
        "means": {metric: statistics.mean(float(row[metric]) for row in rows) for metric in METRICS},
        "totals": {metric: sum(row[metric] for row in rows) for metric in METRICS},
        "recovery_mean": statistics.mean(distances) if distances else None,
        "recovery_max": max(distances) if distances else None,
    }


def _environment() -> dict[str, Any]:
    skill = REPO / "skills/semantic-analysis/SKILL.md"
    installed = Path.home() / ".claude/skills/semantic-analysis/SKILL.md"
    binary = Path("D:/Programs/ast-tool/ast-tool.exe")
    return {
        "revision": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=REPO, text=True).strip(),
        "branch": subprocess.check_output(["git", "branch", "--show-current"], cwd=REPO, text=True).strip(),
        "skill_sha256": hashlib.sha256(skill.read_bytes()).hexdigest(),
        "installed_skill_sha256": hashlib.sha256(installed.read_bytes()).hexdigest(),
        "ast_binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
        "cohort": COHORT,
    }


def main() -> None:
    OUT.mkdir(exist_ok=True)
    runs: list[dict[str, Any]] = []
    annotations: list[dict[str, Any]] = []
    for label, trace_dir, results in [
        ("controlled-phase7d", ROOT / "controlled_phase7d/traces", ROOT / "controlled_phase7d/results.jsonl"),
        ("historical-noskill", REPO / "experimental/traces_noskill", REPO / "experimental/results_noskill.jsonl"),
    ]:
        source_runs, source_annotations = _load_source(label, trace_dir, results)
        runs.extend(source_runs)
        annotations.extend(source_annotations)
    source_runs, source_annotations = _phase7e()
    runs.extend(source_runs)
    annotations.extend(source_annotations)

    by_task: dict[str, dict[str, Any]] = {}
    for task in COHORT:
        task_runs = [row for row in runs if row["task_id"] == task]
        semantic = [row for row in task_runs if row["routing_mode"] == "semantic"]
        nonsemantic = [row for row in task_runs if row["routing_mode"] != "semantic"]
        by_task[task] = {
            "semantic": _stats(semantic), "nonsemantic": _stats(nonsemantic),
            "sources": Counter(row["source"] for row in task_runs),
            "semantic_trajectories": Counter("→".join(row["ast_tool_sequence"]) or "none" for row in semantic),
            "nonsemantic_trajectories": Counter("→".join(row["ast_tool_sequence"]) or "none" for row in nonsemantic),
        }

    command_summary: dict[str, Any] = {}
    # Include malformed/obsolete command tokens as well as the supported
    # commands so the command aggregate accounts for every annotated call.
    for command in sorted({row["command"] for row in annotations}):
        calls = [row for row in annotations if row["command"] == command]
        durations = [row["duration_seconds"] for row in calls if row["duration_seconds"] is not None]
        command_summary[command] = {
            "calls": len(calls), "successes": sum(row["success"] for row in calls),
            "classifications": Counter(row["classification"] for row in calls),
            "tasks": Counter(row["task"] for row in calls),
            "total_duration_seconds": round(sum(durations), 3),
            "mean_duration_seconds": statistics.mean(durations) if durations else None,
            "output_chars": sum(row["output_chars"] for row in calls),
            "search_result_roles": Counter(row["search_result_role"] for row in calls if row["search_result_role"]),
        }

    traces: dict[tuple[str, str], list[dict[str, Any]]] = defaultdict(list)
    for row in annotations:
        traces[(row["task"], row["source"])].append(row)
    short_name_recoveries = []
    search_find = []
    repeated_relationship = []
    for (task, source), calls in traces.items():
        for index, call in enumerate(calls):
            if index + 2 < len(calls):
                triple = calls[index:index + 3]
                if (triple[0]["command"] in ("callers", "callees", "references")
                    and triple[0]["classification"] == "F"
                    and triple[1]["command"] == "search"
                    and triple[2]["command"] == triple[0]["command"]
                    and triple[2]["success"]):
                    short_name_recoveries.append({"task": task, "source": source, "calls": triple})
            if index + 1 < len(calls):
                following = calls[index + 1]
                if call["command"] == "search" and following["command"] == "find":
                    search_find.append({"task": task, "source": source, "calls": [call, following]})
                if (call["command"] == following["command"]
                    and call["command"] in ("callers", "callees", "references")):
                    repeated_relationship.append({
                        "task": task, "source": source, "command": call["command"],
                        "same_target": call["target"] == following["target"],
                        "first_success": call["success"], "second_success": following["success"],
                    })

    patterns = {
        "relationship_resolution": [row for row in annotations if row["classification"] in ("B", "C")],
        "redundant_find": [row for row in annotations if row["classification"] == "D"],
        "find_then_read": [row for row in annotations if row["classification"] == "E"],
        "recovery": [row for row in annotations if row["classification"] == "F"],
    }
    payload = {
        "environment": _environment(), "run_count": len(runs), "runs": runs,
        "task_comparisons": by_task, "annotations": annotations,
        "annotation_counts": Counter(row["classification"] for row in annotations),
        "command_summary": command_summary,
        "pattern_counts": {name: len(rows) for name, rows in patterns.items()},
        "semantic_annotation_counts": Counter(row["classification"] for row in annotations if row["routing_mode"] == "semantic"),
        "direct_ast_annotation_counts": Counter(row["classification"] for row in annotations if row["routing_mode"] == "direct-ast"),
        "short_name_recoveries": short_name_recoveries,
        "search_find_sequences": search_find,
        "repeated_relationship_sequences": repeated_relationship,
    }
    (OUT / "audit.json").write_text(json.dumps(payload, indent=2), encoding="utf-8")
    (OUT / "ast-call-annotations.json").write_text(json.dumps(annotations, indent=2), encoding="utf-8")
    (OUT / "runs.json").write_text(json.dumps(runs, indent=2), encoding="utf-8")
    with (OUT / "ast-call-annotations.csv").open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(annotations[0]))
        writer.writeheader()
        writer.writerows(annotations)
    print(json.dumps({key: payload[key] for key in (
        "environment", "run_count", "annotation_counts", "command_summary", "pattern_counts"
    )}, indent=2))


if __name__ == "__main__":
    main()
