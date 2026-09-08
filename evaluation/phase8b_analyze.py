"""Build Phase 8b raw relationship and controlled-agent measurements."""
from __future__ import annotations

from collections import Counter
import hashlib
import json
from pathlib import Path
import statistics
import subprocess
import sys
import time
from typing import Any

sys.path.insert(0, str(Path(__file__).parent))
from trace_analyzer import analyze_directory


ROOT = Path(__file__).resolve().parent
REPO = ROOT.parent
OUT = ROOT / "phase8b"
BIN = REPO / "bin" / "ast-tool.exe"
TASKS = ["level2-004", "level4-006"]
METRICS = [
    "success", "total_tool_calls", "ast_tool_calls", "ast_tool_failures",
    "ast_tool_retries", "ast_tool_help_calls", "search_calls", "find_calls",
    "callers_calls", "callees_calls", "references_calls", "grep_calls",
    "glob_calls", "read_calls", "bash_calls", "edit_calls", "total_tokens",
    "elapsed_seconds",
]


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def enrich(row: dict[str, Any]) -> dict[str, Any]:
    result = dict(row)
    for command in ("search", "find", "callers", "callees", "references"):
        result[f"{command}_calls"] = row.get("ast_tool_commands", {}).get(command, 0)
    return result


def mean_metrics(rows: list[dict[str, Any]]) -> dict[str, float]:
    values = [enrich(row) for row in rows]
    return {metric: statistics.mean(float(row.get(metric, 0)) for row in values)
            for metric in METRICS}


def load_agent_runs() -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    for directory in sorted((OUT / "agent").glob("r[0-9]*"), key=lambda p: int(p.name[1:])):
        summaries = analyze_directory(
            directory / "traces", results_path=directory / "results.jsonl",
            task_filter=TASKS,
        )
        for row in summaries:
            row["repeat"] = int(directory.name[1:])
            row["trajectory"] = "→".join(row["ast_tool_sequence"]) or "none"
            row["skill_first"] = bool(row["tool_sequence"] and row["tool_sequence"][0] == "Skill")
            rows.append(row)
    return rows


def run_command(command: str, target: str, root: str) -> dict[str, Any]:
    start = time.perf_counter()
    result = subprocess.run(
        [str(BIN), command, target, root], cwd=REPO, capture_output=True,
        text=True, encoding="utf-8",
    )
    return {
        "returncode": result.returncode,
        "elapsed_seconds": round(time.perf_counter() - start, 6),
        "stdout_lines": [line for line in result.stdout.splitlines() if line.strip()],
        "stderr_lines": [line for line in result.stderr.splitlines() if line.strip()],
    }


def before_output(task: str, command: str) -> list[str]:
    trace = ROOT / "controlled_phase7d" / "traces" / f"{task}.jsonl"
    for line in trace.read_text(encoding="utf-8").splitlines():
        event = json.loads(line)
        meta = event.get("ast_tool", {})
        if event.get("event") == "tool_call" and meta.get("command") == command:
            return [item for item in str(event.get("output", "")).splitlines() if item.strip()]
    raise AssertionError((task, command))


def direct_replays() -> list[dict[str, Any]]:
    cases = [
        ("level2-004", "callers", "auth::AuthToken::validate",
         "evaluation/repositories/level2-auth",
         ["auth::AuthService::login", "web::AuthController::handleLogin",
          "web::AuthController::handleRefresh", "web::SessionController::handle"]),
        ("level2-004", "references", "auth::AuthToken::validate",
         "evaluation/repositories/level2-auth", ["auth_service.cpp:14:19",
          "auth_controller.cpp:13:17", "auth_controller.cpp:26:17",
          "session_controller.cpp:12:17"]),
        ("level4-006", "callers", "service::ValidationService::validate",
         "evaluation/repositories/level4-api", ["processor::RequestProcessor::process"]),
        ("level4-006", "references", "service::ValidationService::validate",
         "evaluation/repositories/level4-api", ["request_processor.cpp:13:21"]),
    ]
    output = []
    for task, command, target, root, expected_fragments in cases:
        runs = [run_command(command, target, root) for _ in range(5)]
        actual = runs[0]["stdout_lines"]
        missing = [fragment for fragment in expected_fragments
                   if not any(fragment in line for line in actual)]
        unexpected = [line for line in actual
                      if not any(fragment in line for fragment in expected_fragments)]
        stable = all(run["returncode"] == 0 and run["stdout_lines"] == actual for run in runs)
        output.append({
            "task": task, "command": command, "target": target, "root": root,
            "before_lines": before_output(task, command),
            "expected_fragments": expected_fragments, "actual_lines": actual,
            "missing": missing, "unexpected": unexpected,
            "stable_across_five": stable,
            "mean_elapsed_seconds": statistics.mean(run["elapsed_seconds"] for run in runs),
            "runs": runs,
        })
    return output


def fixture_measurements() -> list[dict[str, Any]]:
    root = "test/ast-member-receiver/workspace"
    cases = [
        ("typed-receiver-callers", "callers", "typed::Validator::validate",
         ["Session::fieldObject", "Session::fieldPointer", "typed::localObject",
          "typed::referenceParameter", "typed::pointerParameter"],
         ["unrelatedField"]),
        ("field-references", "references", "typed::Validator::validate",
         ["typed_receivers.cpp:16:37", "typed_receivers.cpp:17:43",
          "typed_receivers.cpp:23:15", "typed_receivers.cpp:27:15",
          "typed_receivers.cpp:31:16"], ["18:36"]),
        ("field-callees", "callees", "typed::Session::fieldObject",
         ["typed::Validator::validate"], []),
        ("pointer-field-callees", "callees", "typed::Session::fieldPointer",
         ["typed::Validator::validate"], []),
        ("local-object-callees", "callees", "typed::localObject",
         ["typed::Validator::validate"], []),
        ("reference-parameter-callees", "callees", "typed::referenceParameter",
         ["typed::Validator::validate"], []),
        ("pointer-parameter-callees", "callees", "typed::pointerParameter",
         ["typed::Validator::validate"], []),
        ("unrelated-type", "callers", "typed::OtherValidator::validate",
         ["Session::unrelatedField"], ["fieldObject", "fieldPointer"]),
        ("namespace-left", "callers", "left::Validator::validate",
         ["NamespaceSession::callLeft"], ["callRight"]),
        ("namespace-right", "callers", "right::Validator::validate",
         ["NamespaceSession::callRight"], ["callLeft"]),
        ("local-field-name-shadow-field", "callers", "shadow_guard::Validator::validate",
         ["Holder::fieldCall"], ["localWithFieldName"]),
        ("local-field-name-shadow-local", "callers", "shadow_guard::OtherValidator::validate",
         ["localWithFieldName"], ["Holder::fieldCall"]),
        ("sibling-scope-hidden", "callers", "scope_guard::Validator::validate",
         [], ["scope_guard::siblingBlocks"]),
        ("sibling-scope-visible", "callers", "scope_guard::OtherValidator::validate",
         ["scope_guard::siblingBlocks"], []),
        ("overload-fails-closed", "callers", "typed::Overloaded::validate", [],
         ["ambiguousMember"]),
        ("complex-receiver-fails-closed", "callees", "typed::unresolvedExpression",
         ["typed::makeValidator"], ["Validator::validate"]),
    ]
    rows = []
    for name, command, target, expected, forbidden in cases:
        run = run_command(command, target, root)
        lines = run["stdout_lines"]
        rows.append({
            "case": name, "command": command, "target": target,
            "expected_fragments": expected, "forbidden_fragments": forbidden,
            "actual_lines": lines,
            "missing": [x for x in expected if not any(x in line for line in lines)],
            "unexpected": [x for x in forbidden if any(x in line for line in lines)],
            "returncode": run["returncode"],
        })
    return rows


def existing_relationship_replays() -> list[dict[str, Any]]:
    cases = [
        ("callees", "scSource", "test/ast-callees/workspace", ["scTarget"]),
        ("callees", "mcSource", "test/ast-callees/workspace", ["mcAlpha", "mcBeta"]),
        ("callees", "ncNestedSource", "test/ast-callees/workspace", ["ncPrint", "ncGet"]),
        ("callees", "ncTransitive", "test/ast-callees/workspace", ["ncNestedSource"]),
        ("callees", "nsCeSource", "test/ast-callees/workspace", ["NsCe::nsTarget"]),
        ("callers", "DdCalClass::ddCalMethod", "test/ast-callers/workspace", ["ddCalCaller"]),
    ]
    rows = []
    for command, target, root, expected in cases:
        runs = [run_command(command, target, root) for _ in range(5)]
        lines = runs[0]["stdout_lines"]
        rows.append({
            "command": command, "target": target, "expected_fragments": expected,
            "actual_lines": lines,
            "missing": [x for x in expected if not any(x in line for line in lines)],
            "unexpected_count": max(0, len(lines) - len(expected)),
            "stable_across_five": all(run["returncode"] == 0
                                      and run["stdout_lines"] == lines for run in runs),
            "runs": runs,
        })
    return rows


def collect_edit_inputs(value: Any, output: list[dict[str, Any]]) -> None:
    if isinstance(value, dict):
        if value.get("name") == "Edit" and isinstance(value.get("input"), dict):
            output.append(value["input"])
        for child in value.values():
            collect_edit_inputs(child, output)
    elif isinstance(value, list):
        for child in value:
            collect_edit_inputs(child, output)


def level4_intended_edit_runs() -> int:
    expected = {
        "src/service/validation_service.h": "validatePayload",
        "src/service/validation_service.cpp": "ValidationService::validatePayload",
        "src/processor/request_processor.cpp": "validator_.validatePayload",
    }
    correct = 0
    for repeat in range(1, 6):
        edits: list[dict[str, Any]] = []
        for session in (OUT / "agent" / f"r{repeat}" / "sessions" / "level4-006").glob("**/*.jsonl"):
            for line in session.read_text(encoding="utf-8").splitlines():
                collect_edit_inputs(json.loads(line), edits)
        matched = set()
        for edit in edits:
            path = str(edit.get("file_path", "")).replace("\\", "/")
            new = str(edit.get("new_string", ""))
            for suffix, fragment in expected.items():
                if path.endswith(suffix) and fragment in new:
                    matched.add(suffix)
        if matched == set(expected) and len(edits) == 3:
            correct += 1
    return correct


def main() -> None:
    runs = load_agent_runs()
    baseline = json.loads((ROOT / "phase7f" / "audit.json").read_text(encoding="utf-8"))
    tasks: dict[str, Any] = {}
    for task in TASKS:
        after = [row for row in runs if row["task_id"] == task]
        before_rows = [row for row in baseline["runs"]
                       if row["task_id"] == task and row.get("routing_mode") == "semantic"]
        before_means = mean_metrics(before_rows)
        after_means = mean_metrics(after)
        distances = [distance for row in after
                     for distance in row["ast_tool_recovery_distances"]]
        tasks[task] = {
            "before_runs": len(before_rows), "after_runs": len(after),
            "before_means": before_means, "after_means": after_means,
            "after_minus_before": {metric: after_means[metric] - before_means[metric]
                                   for metric in METRICS},
            "before_trajectories": Counter("→".join(row["ast_tool_sequence"])
                                           for row in before_rows),
            "after_trajectories": Counter(row["trajectory"] for row in after),
            "validation_successes": sum(bool(row["success"]) for row in after),
            "skill_first_runs": sum(bool(row["skill_first"]) for row in after),
            "recovery_mean": statistics.mean(distances) if distances else None,
            "recovery_max": max(distances) if distances else None,
        }

    manifest = json.loads((OUT / "agent" / "manifest.json").read_text(encoding="utf-8"))
    payload = {
        "environment": {
            "revision": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=REPO,
                                                text=True).strip(),
            "branch": subprocess.check_output(["git", "branch", "--show-current"], cwd=REPO,
                                              text=True).strip(),
            "phase8a_final_binary_sha256":
                "68d5154a86b6afb0142c3a12906a88b715accfb0a2f5a6fe63254d0de6ec16d1",
            "agent_binary_sha256": manifest["workspace_binary_sha256"],
            "final_binary_sha256": sha256(BIN),
            "skill_sha256": sha256(REPO / "skills/semantic-analysis/SKILL.md"),
            "installed_skill_sha256": sha256(Path.home() / ".claude/skills/semantic-analysis/SKILL.md"),
        },
        "agent_runs": runs,
        "task_aggregates": tasks,
        "agent_manifest_checks": manifest["checks"],
        "direct_replays": direct_replays(),
        "fixture_measurements": fixture_measurements(),
        "existing_relationship_replays": existing_relationship_replays(),
        "level4_validator_defect": {
            "validation_failures": 5,
            "reason": "Committed ApiHandler declaration lacks processor_ and registry_ fields used by api_handler.cpp",
            "intended_three_edits_observed_in_sessions": level4_intended_edit_runs(),
        },
    }
    (OUT / "measurements.json").write_text(json.dumps(payload, indent=2), encoding="utf-8")
    print(json.dumps({"environment": payload["environment"],
                      "task_aggregates": tasks,
                      "direct_replays": payload["direct_replays"],
                      "fixture_measurements": payload["fixture_measurements"]}, indent=2))


if __name__ == "__main__":
    main()
