"""Phase 9b.2 aggregate analysis."""
from __future__ import annotations

import json
import re
import statistics
from collections import defaultdict, Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parent
OUT = ROOT / "phase9b2"
AGENT = OUT / "agent"

MANIFEST = json.loads((OUT / "manifest.json").read_text(encoding="utf-8"))
CHECKS = MANIFEST["checks"]
ALL_TASKS = MANIFEST["all_tasks"]

REPEATED_TASKS = sorted({c["task"] for c in CHECKS if c["repeat"] > 1})
GUARD_TASKS = ["level1-001", "level1-003", "level2-001", "level3-001"]
PHASE8_TASKS = ["level2-008", "level3-008", "level2-004", "level4-006", "level1-002", "level2-005"]

METRICS = [
    "total_tool_calls", "ast_tool_calls", "ast_tool_failures", "ast_tool_retries",
    "ast_tool_help_calls", "read_calls", "grep_calls", "glob_calls", "bash_calls",
    "edit_calls", "skill_calls", "total_tokens", "elapsed_seconds",
]


def mean(xs):
    xs = [x for x in xs if x is not None]
    return round(statistics.mean(xs), 3) if xs else None


def median(xs):
    xs = [x for x in xs if x is not None]
    return round(statistics.median(xs), 3) if xs else None


def load_analysis(task: str, arm: str, repeat: int) -> dict | None:
    p = AGENT / task / f"r{repeat}" / f"arm_{arm}" / "analysis" / f"{task}.json"
    if not p.exists():
        return None
    return json.loads(p.read_text(encoding="utf-8"))


def load_trace_events(task: str, arm: str, repeat: int) -> list[dict]:
    p = AGENT / task / f"r{repeat}" / f"arm_{arm}" / "traces" / f"{task}.jsonl"
    events = []
    if not p.exists():
        return events
    with p.open("r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            ev = json.loads(line)
            if ev.get("event") == "tool_call":
                events.append(ev)
    return events


def n_repeats(task: str) -> int:
    return max(c["repeat"] for c in CHECKS if c["task"] == task)


# ---------------------------------------------------------------------------
# 1. Stage-1-only whole-suite aggregate (82 runs, one per task per arm)
# ---------------------------------------------------------------------------

def stage1_aggregate() -> dict:
    out = {}
    for arm in ("A", "B"):
        runs = [load_analysis(t, arm, 1) for t in ALL_TASKS]
        runs = [r for r in runs if r]
        n = len(runs)
        successes = sum(1 for r in runs if r.get("success"))
        entry = {"n_runs": n, "success_rate": round(successes / n, 3)}
        for m in METRICS:
            entry[m] = mean([r.get(m) for r in runs])
        cmd_totals = defaultdict(int)
        for r in runs:
            for cmd, cnt in (r.get("ast_tool_commands") or {}).items():
                cmd_totals[cmd] += cnt
        entry["ast_commands_total"] = dict(cmd_totals)
        out[arm] = entry
    return out


# ---------------------------------------------------------------------------
# 2. Task-level table (all 41 tasks, stage1 run=1 vs vs mean over available repeats)
# ---------------------------------------------------------------------------

def task_level_table() -> list[dict]:
    rows = []
    for task in ALL_TASKS:
        nrep = n_repeats(task)
        row = {"task": task, "n_repeats": nrep}
        for arm in ("A", "B"):
            runs = [load_analysis(task, arm, r) for r in range(1, nrep + 1)]
            runs = [r for r in runs if r]
            n = len(runs)
            successes = sum(1 for r in runs if r.get("success"))
            e = {"n": n, "success_rate": round(successes / n, 3) if n else None}
            for m in METRICS:
                e[m] = mean([r.get(m) for r in runs])
            row[f"arm_{arm}"] = e
        rows.append(row)
    return rows


# ---------------------------------------------------------------------------
# 3. Skill invocation / routing analysis
# ---------------------------------------------------------------------------

def routing_breakdown(checks: list[dict]) -> dict:
    by_arm_cat = Counter()
    by_arm_sem = Counter()
    by_arm_sem_pos = defaultdict(list)
    other_skill = Counter()
    for c in checks:
        r = c["routing"]
        by_arm_cat[(c["arm"], r.get("category"))] += 1
        by_arm_sem[(c["arm"], r.get("semantic_analysis_invoked"))] += 1
        if r.get("semantic_analysis_first_position"):
            by_arm_sem_pos[c["arm"]].append(r["semantic_analysis_first_position"])
        if r.get("other_skill_invoked"):
            other_skill[c["arm"]] += 1
    return {
        "by_arm_category": {f"{a}:{cat}": v for (a, cat), v in by_arm_cat.items()},
        "by_arm_semantic_invoked": {f"{a}:{s}": v for (a, s), v in by_arm_sem.items()},
        "mean_first_position_when_invoked": {a: mean(v) for a, v in by_arm_sem_pos.items()},
        "other_skill_invoked_count": dict(other_skill),
    }


def exact_skill_cohorts() -> dict:
    """Classify each task by whether semantic-analysis was invoked in A/B across its runs."""
    cohorts = {"both": [], "neither": [], "only_A": [], "only_B": [], "mixed": []}
    per_task = {}
    for task in ALL_TASKS:
        nrep = n_repeats(task)
        a_rate = 0
        b_rate = 0
        for arm, counter in (("A", "a_rate"), ("B", "b_rate")):
            pass
        a_invoked = []
        b_invoked = []
        for r in range(1, nrep + 1):
            ca = next((c for c in CHECKS if c["task"] == task and c["arm"] == "A" and c["repeat"] == r), None)
            cb = next((c for c in CHECKS if c["task"] == task and c["arm"] == "B" and c["repeat"] == r), None)
            if ca:
                a_invoked.append(bool(ca["routing"].get("semantic_analysis_invoked")))
            if cb:
                b_invoked.append(bool(cb["routing"].get("semantic_analysis_invoked")))
        a_rate = sum(a_invoked) / len(a_invoked) if a_invoked else 0
        b_rate = sum(b_invoked) / len(b_invoked) if b_invoked else 0
        per_task[task] = {"a_rate": round(a_rate, 2), "b_rate": round(b_rate, 2), "n_repeats": nrep}
        if a_rate == 0 and b_rate == 0:
            cohorts["neither"].append(task)
        elif a_rate == 1 and b_rate == 1:
            cohorts["both"].append(task)
        elif a_rate > 0 and b_rate == 0:
            cohorts["only_A"].append(task)
        elif b_rate > 0 and a_rate == 0:
            cohorts["only_B"].append(task)
        else:
            cohorts["mixed"].append(task)
    return {"cohorts": cohorts, "per_task": per_task}


# ---------------------------------------------------------------------------
# 4. Phase 8a/8b/8c normal-routing signal (regex-anchored, reused from 9b.1)
# ---------------------------------------------------------------------------

TARGETS = {
    "level2-008": ("callers", "AuthToken::expire"),
    "level3-008": ("callers", "DataStore::save"),
    "level2-004": ("callers", "AuthToken::validate"),
    "level4-006": ("callers", "ValidationService::validate"),
    "level1-002": ("references", "InventoryService::save"),
    "level2-005": ("callees", "AuthService::refresh"),
}


def phase8_signal() -> dict:
    out = {}
    for task, (cmd_filter, sym) in TARGETS.items():
        empty_re = re.compile(rf"no {cmd_filter} found for: [\w:]*{re.escape(sym)}")
        err_re = re.compile(rf"symbol not found: [\w:]*{re.escape(sym)}")
        nrep = n_repeats(task)
        per_arm = {}
        for arm in ("A", "B"):
            occ = 0
            invoked = 0
            for r in range(1, nrep + 1):
                events = load_trace_events(task, arm, r)
                ast_events = [e for e in events if e.get("ast_tool")]
                hit_cmd = any(e["ast_tool"]["command"] == cmd_filter for e in ast_events)
                if hit_cmd:
                    invoked += 1
                for e in ast_events:
                    if e["ast_tool"]["command"] != cmd_filter:
                        continue
                    out_text = e.get("output", "")
                    if empty_re.search(out_text) or err_re.search(out_text):
                        occ += 1
                        break
            per_arm[arm] = {"occurrences": occ, "n_repeats": nrep, "cmd_invoked_runs": invoked}
        out[task] = per_arm
    return out


# ---------------------------------------------------------------------------
# 5. Windows path-quoting artifact detection
# ---------------------------------------------------------------------------

PATH_ARTIFACT_RE = re.compile(r"error: workspace at '[A-Za-z]:[A-Za-z0-9_/\\]+'")


def path_artifact_scan() -> list[dict]:
    hits = []
    for task in ALL_TASKS:
        nrep = n_repeats(task)
        for arm in ("A", "B"):
            for r in range(1, nrep + 1):
                events = load_trace_events(task, arm, r)
                for e in events:
                    out_text = e.get("output", "")
                    if PATH_ARTIFACT_RE.search(out_text) or ("error: workspace at" in out_text and "MyDocuments" in out_text and "\\" not in out_text.split("error: workspace at")[1][:40]):
                        hits.append({"task": task, "arm": arm, "repeat": r, "command": (e.get("ast_tool") or {}).get("command")})
                        break
    return hits


# ---------------------------------------------------------------------------
# 6. Residual gap (repo_.update / UserRepository::update) tracking on level2-005
# ---------------------------------------------------------------------------

def residual_gap_scan() -> dict:
    out = {"A": [], "B": []}
    nrep = n_repeats("level2-005")
    for arm in ("A", "B"):
        for r in range(1, nrep + 1):
            events = load_trace_events("level2-005", arm, r)
            ast_events = [e for e in events if e.get("ast_tool")]
            callees_calls = [e for e in ast_events if e["ast_tool"]["command"] == "callees"]
            found_update_via_callees = any("UserRepository::update" in e.get("output", "") for e in callees_calls)
            used_callees = bool(callees_calls)
            out[arm].append({
                "repeat": r,
                "used_callees": used_callees,
                "found_repo_update_via_callees": found_update_via_callees,
            })
    return out


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> None:
    summary = {
        "stage1_aggregate": stage1_aggregate(),
        "task_level": task_level_table(),
        "routing_stage1": routing_breakdown([c for c in CHECKS if c["repeat"] == 1]),
        "routing_all": routing_breakdown(CHECKS),
        "exact_skill_cohorts": exact_skill_cohorts(),
        "phase8_signal": phase8_signal(),
        "path_artifacts": path_artifact_scan(),
        "residual_gap_level2_005": residual_gap_scan(),
        "repeated_tasks": REPEATED_TASKS,
    }
    (OUT / "summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
    print(f"Wrote {OUT / 'summary.json'}")

    print("\n=== stage1 aggregate ===")
    for arm, e in summary["stage1_aggregate"].items():
        print(arm, {k: e[k] for k in ("n_runs", "success_rate", "total_tool_calls", "ast_tool_calls",
                                       "total_tokens", "elapsed_seconds")})
    print("\n=== exact skill cohorts ===")
    for k, v in summary["exact_skill_cohorts"]["cohorts"].items():
        print(k, v)
    print("\n=== path artifacts ===", len(summary["path_artifacts"]))
    for h in summary["path_artifacts"]:
        print(" ", h)


if __name__ == "__main__":
    main()
