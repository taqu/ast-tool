"""Phase 9b.1 aggregate analysis.

Reads evaluation/phase9b1/agent/<task>/r<repeat>/arm_<X>/{analysis,traces}
and evaluation/phase9b1/{manifest.json,direct_probes.json}, and writes a
consolidated evaluation/phase9b1/summary.json used to write the markdown
report.
"""
from __future__ import annotations

import json
import statistics
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent
OUT = ROOT / "phase9b1"
AGENT = OUT / "agent"

TASK_GROUP = {
    "level2-008": "A", "level3-008": "A",
    "level2-004": "B", "level4-006": "B", "level1-002": "B",
    "level2-005": "C",
    "level1-001": "D", "level1-003": "D", "level2-001": "D", "level3-001": "D",
}
ALL_TASKS = list(TASK_GROUP.keys())

METRICS = [
    "total_tool_calls", "ast_tool_calls", "ast_tool_failures", "ast_tool_retries",
    "ast_tool_help_calls", "read_calls", "grep_calls", "glob_calls", "bash_calls",
    "edit_calls", "skill_calls", "total_tokens", "elapsed_seconds",
]

AST_COMMANDS = ["search", "find", "callers", "callees", "references", "symbols"]


def load_runs(task: str, arm: str) -> list[dict]:
    runs = []
    for d in sorted(AGENT.glob(f"{task}/r*/arm_{arm}")):
        analysis_path = d / "analysis" / f"{task}.json"
        if analysis_path.exists():
            rec = json.loads(analysis_path.read_text(encoding="utf-8"))
            rec["_repeat"] = d.parent.name
            runs.append(rec)
    return runs


def mean(xs):
    xs = [x for x in xs if x is not None]
    return round(statistics.mean(xs), 3) if xs else None


def median(xs):
    xs = [x for x in xs if x is not None]
    return round(statistics.median(xs), 3) if xs else None


def task_summary(task: str) -> dict:
    out = {"task": task, "group": TASK_GROUP[task]}
    for arm in ("A", "B"):
        runs = load_runs(task, arm)
        n = len(runs)
        successes = sum(1 for r in runs if r.get("success"))
        entry = {
            "n_runs": n,
            "success_rate": round(successes / n, 3) if n else None,
        }
        for m in METRICS:
            vals = [r.get(m) for r in runs]
            entry[m] = {
                "mean": mean(vals), "median": median(vals),
                "min": min(vals) if vals else None, "max": max(vals) if vals else None,
                "values": vals,
            }
        cmd_counts = defaultdict(list)
        for r in runs:
            commands = r.get("ast_tool_commands", {})
            for cmd in AST_COMMANDS:
                cmd_counts[cmd].append(commands.get(cmd, 0))
        entry["ast_commands"] = {c: mean(v) for c, v in cmd_counts.items()}
        recov = []
        for r in runs:
            recov.extend(r.get("ast_tool_recovery_distances") or [])
        entry["recovery_mean"] = mean(recov) if recov else None
        entry["recovery_max"] = max(recov) if recov else None
        out[f"arm_{arm}"] = entry
    return out


def delta(a, b):
    if a is None or b is None:
        return None
    return round(b - a, 3)


def aggregate_across(tasks: list[str], arm: str) -> dict:
    """Aggregate raw per-run values across a set of tasks for one arm."""
    all_runs = []
    for t in tasks:
        all_runs.extend(load_runs(t, arm))
    n = len(all_runs)
    successes = sum(1 for r in all_runs if r.get("success"))
    out = {"n_runs": n, "success_rate": round(successes / n, 3) if n else None}
    for m in METRICS:
        vals = [r.get(m) for r in all_runs]
        out[m] = mean(vals)
    cmd_totals = defaultdict(int)
    for r in all_runs:
        for cmd, cnt in (r.get("ast_tool_commands") or {}).items():
            cmd_totals[cmd] += cnt
    out["ast_commands_total"] = dict(cmd_totals)
    recov = []
    for r in all_runs:
        recov.extend(r.get("ast_tool_recovery_distances") or [])
    out["recovery_mean"] = mean(recov)
    out["recovery_max"] = max(recov) if recov else None
    return out


# ---------------------------------------------------------------------------
# Causal pattern detection from raw traces
# ---------------------------------------------------------------------------

def load_trace_events(task: str, repeat_dir_name: str, arm: str) -> list[dict]:
    path = AGENT / task / repeat_dir_name / f"arm_{arm}" / "traces" / f"{task}.jsonl"
    events = []
    if not path.exists():
        return events
    with path.open("r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            ev = json.loads(line)
            if ev.get("event") == "tool_call" and ev.get("ast_tool"):
                events.append({
                    "command": ev["ast_tool"]["command"],
                    "success": ev.get("success"),
                    "output": (ev.get("output") or "")[:200],
                })
    return events


def _is_empty_note(output: str) -> bool:
    return "note: no " in output and "found" in output


def _is_not_found_error(output: str) -> bool:
    return "symbol not found" in output or "symbol '" in output and "not found" in output


def classify_run(events: list[dict]) -> dict:
    """Classify a run's ast_tool event sequence for Phase 8a/8b/8c causal patterns."""
    pattern_8a = 0   # relationship(partial/unresolved) -> failure -> search -> relationship(exact) success
    pattern_8b = 0   # relationship query on correct FQN -> empty note (receiver-type gap)
    pattern_8c = 0   # callees on decl-only target -> empty note
    relationship_cmds = {"callers", "callees", "references"}

    for i, ev in enumerate(events):
        if ev["command"] not in relationship_cmds:
            continue
        out = ev["output"]
        if ev["success"] is False and _is_not_found_error(out):
            # look ahead for a search then a same-command success
            for j in range(i + 1, len(events)):
                if events[j]["command"] == "search":
                    for k in range(j + 1, len(events)):
                        if events[k]["command"] == ev["command"]:
                            if events[k]["success"] and not _is_empty_note(events[k]["output"]):
                                pattern_8a += 1
                            break
                    break
        elif ev["success"] is True and _is_empty_note(out):
            if ev["command"] == "callees":
                pattern_8c += 1
            else:
                pattern_8b += 1

    return {"pattern_8a": pattern_8a, "pattern_8b": pattern_8b, "pattern_8c": pattern_8c}


def causal_patterns() -> dict:
    result = {}
    for task in ALL_TASKS:
        result[task] = {}
        for arm in ("A", "B"):
            counts = {"pattern_8a": 0, "pattern_8b": 0, "pattern_8c": 0, "runs": 0}
            for d in sorted(AGENT.glob(f"{task}/r*/arm_{arm}")):
                events = load_trace_events(task, d.parent.name, arm)
                if not events and not d.exists():
                    continue
                counts["runs"] += 1
                c = classify_run(events)
                for k in ("pattern_8a", "pattern_8b", "pattern_8c"):
                    counts[k] += c[k]
            result[task][arm] = counts
    return result


# ---------------------------------------------------------------------------
# Routing / binary integrity summary
# ---------------------------------------------------------------------------

def manifest_integrity() -> dict:
    manifest = json.loads((OUT / "manifest.json").read_text(encoding="utf-8"))
    checks = manifest.get("checks", [])
    routing_failures = [c for c in checks if not c.get("routing", {}).get("routing_ok")]
    binary_failures = [c for c in checks if not c.get("binary_stable")]
    return {
        "total_checks": len(checks),
        "routing_failures": routing_failures,
        "binary_failures": binary_failures,
        "binary_sha256": manifest["binary_sha256"],
        "binary_git_revision": manifest["binary_git_revision"],
        "skill_sha256": manifest["skill_sha256"],
    }


def main() -> None:
    summary = {"tasks": {}, "groups": {}, "causal": causal_patterns(), "integrity": manifest_integrity()}

    for task in ALL_TASKS:
        summary["tasks"][task] = task_summary(task)

    affected = [t for t, g in TASK_GROUP.items() if g in ("A", "B", "C")]
    guards = [t for t, g in TASK_GROUP.items() if g == "D"]

    for label, tasks in [
        ("all", ALL_TASKS), ("affected", affected), ("guards", guards),
        ("group_A", [t for t, g in TASK_GROUP.items() if g == "A"]),
        ("group_B", [t for t, g in TASK_GROUP.items() if g == "B"]),
        ("group_C", [t for t, g in TASK_GROUP.items() if g == "C"]),
        ("group_D", guards),
    ]:
        summary["groups"][label] = {
            "A": aggregate_across(tasks, "A"),
            "B": aggregate_across(tasks, "B"),
        }

    (OUT / "summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
    print(f"Wrote {OUT / 'summary.json'}")

    # console preview
    print("\n=== Aggregate (all tasks) ===")
    for arm in ("A", "B"):
        g = summary["groups"]["all"][arm]
        print(arm, {k: g[k] for k in ("n_runs", "success_rate", "total_tool_calls",
                                       "ast_tool_calls", "ast_tool_failures",
                                       "ast_tool_retries", "total_tokens", "elapsed_seconds")})


if __name__ == "__main__":
    main()
