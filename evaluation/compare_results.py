#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import json
import statistics
from collections import Counter, defaultdict
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


LEVEL_ORDER = [
    "smoke",
    "level1",
    "level2",
    "level3",
    "level4",
    "level5",
]


# ---------------------------------------------------------------------------
# Data model
# ---------------------------------------------------------------------------

@dataclass
class NormalizedResult:
    task_id: str
    level: str
    success: bool
    elapsed_seconds: float | None
    tokens: dict[str, int | None]
    tools: dict[str, int]
    ast_tool: dict[str, int]
    workflow: list[dict[str, Any]]
    changed_files: list[str]
    validation: dict[str, Any] | None


@dataclass
class TaskComparison:
    task_id: str
    level: str
    with_ast: NormalizedResult | None
    without_ast: NormalizedResult | None


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def get_level(task_id: str) -> str:
    if "-" not in task_id:
        return "unknown"
    return task_id.split("-", 1)[0]


def safe_mean(values: list[float]) -> float | None:
    if not values:
        return None
    return statistics.mean(values)


def safe_median(values: list[float]) -> float | None:
    if not values:
        return None
    return statistics.median(values)


def fmt(value: float | None, decimals: int = 2) -> str:
    if value is None:
        return ""
    return f"{value:.{decimals}f}"


def delta(a: float | None, b: float | None) -> float | None:
    if a is None or b is None:
        return None
    return a - b


# ---------------------------------------------------------------------------
# Loading
# ---------------------------------------------------------------------------

def load_jsonl(path: Path) -> list[dict[str, Any]]:
    records: list[dict[str, Any]] = []

    with path.open("r", encoding="utf-8") as f:
        for line_number, line in enumerate(f, start=1):
            line = line.strip()

            if not line:
                continue

            try:
                data = json.loads(line)
            except json.JSONDecodeError as e:
                print(f"WARNING: invalid JSON at {path}:{line_number}: {e}")
                continue

            if not isinstance(data, dict):
                print(f"WARNING: expected object at {path}:{line_number}")
                continue

            records.append(data)

    return records


def select_latest_results(
    records: list[dict[str, Any]],
) -> dict[str, dict[str, Any]]:
    latest: dict[str, dict[str, Any]] = {}

    for record in records:
        task_id = str(record.get("task_id", "unknown"))
        latest[task_id] = record

    return latest


def normalize_result(record: dict[str, Any]) -> NormalizedResult:
    task_id = str(record.get("task_id", "unknown"))

    level_raw = record.get("level")
    level = str(level_raw) if level_raw else get_level(task_id)

    success = bool(record.get("success", False))

    elapsed_raw = record.get("elapsed_seconds")
    elapsed_seconds: float | None = (
        float(elapsed_raw) if elapsed_raw is not None else None
    )

    tokens_raw = record.get("tokens") or {}
    tokens: dict[str, int | None] = {}
    for key in ("input", "output", "cache_read", "cache_creation"):
        raw = tokens_raw.get(key)
        tokens[key] = int(raw) if raw is not None else None

    tools = {k: int(v) for k, v in (record.get("tools") or {}).items()}
    ast_tool = {k: int(v) for k, v in (record.get("ast_tool") or {}).items()}

    workflow_raw = record.get("workflow") or []
    workflow: list[dict[str, Any]] = (
        list(workflow_raw) if isinstance(workflow_raw, list) else []
    )

    changed_files_raw = record.get("changed_files") or []
    changed_files: list[str] = (
        list(changed_files_raw) if isinstance(changed_files_raw, list) else []
    )

    validation_raw = record.get("validation")
    validation: dict[str, Any] | None = (
        dict(validation_raw) if isinstance(validation_raw, dict) else None
    )

    return NormalizedResult(
        task_id=task_id,
        level=level,
        success=success,
        elapsed_seconds=elapsed_seconds,
        tokens=tokens,
        tools=tools,
        ast_tool=ast_tool,
        workflow=workflow,
        changed_files=changed_files,
        validation=validation,
    )


def load_normalized(path: Path) -> dict[str, NormalizedResult]:
    records = load_jsonl(path)
    latest = select_latest_results(records)
    return {
        task_id: normalize_result(record)
        for task_id, record in latest.items()
    }


# ---------------------------------------------------------------------------
# Task comparison
# ---------------------------------------------------------------------------

def build_task_comparisons(
    with_ast: dict[str, NormalizedResult],
    without_ast: dict[str, NormalizedResult],
) -> list[TaskComparison]:
    all_ids = set(with_ast) | set(without_ast)
    comparisons: list[TaskComparison] = []

    for task_id in sorted(all_ids):
        w = with_ast.get(task_id)
        wo = without_ast.get(task_id)
        level = (w or wo).level  # type: ignore[union-attr]
        comparisons.append(TaskComparison(
            task_id=task_id,
            level=level,
            with_ast=w,
            without_ast=wo,
        ))

    return comparisons


def matched(comparisons: list[TaskComparison]) -> list[TaskComparison]:
    return [c for c in comparisons if c.with_ast and c.without_ast]


def unmatched(comparisons: list[TaskComparison]) -> list[TaskComparison]:
    return [c for c in comparisons if not (c.with_ast and c.without_ast)]


# ---------------------------------------------------------------------------
# Aggregation helpers
# ---------------------------------------------------------------------------

def _levels_in(comparisons: list[TaskComparison]) -> list[str]:
    found = {c.level for c in comparisons}
    ordered = [lv for lv in LEVEL_ORDER if lv in found]
    extras = sorted(found - set(LEVEL_ORDER))
    return ordered + extras


def _total_tokens(tokens: dict[str, int | None]) -> int | None:
    values = [tokens.get(k) for k in ("input", "output", "cache_read", "cache_creation")]
    if all(v is None for v in values):
        return None
    return sum(v for v in values if v is not None)


# ---------------------------------------------------------------------------
# Per-task CSV
# ---------------------------------------------------------------------------

def _all_tool_names(comparisons: list[TaskComparison]) -> list[str]:
    names: set[str] = set()
    for c in comparisons:
        if c.with_ast:
            names.update(c.with_ast.tools)
        if c.without_ast:
            names.update(c.without_ast.tools)
    return sorted(names)


def write_per_task_csv(
    comparisons: list[TaskComparison],
    path: Path,
) -> None:
    fieldnames = [
        "task_id",
        "level",
        "with_ast_present",
        "without_ast_present",
        "with_ast_success",
        "without_ast_success",
        "success_changed",
        "with_ast_elapsed_seconds",
        "without_ast_elapsed_seconds",
        "elapsed_delta_seconds",
        "with_ast_input_tokens",
        "without_ast_input_tokens",
        "input_tokens_delta",
        "with_ast_output_tokens",
        "without_ast_output_tokens",
        "output_tokens_delta",
        "with_ast_total_tokens",
        "without_ast_total_tokens",
        "total_tokens_delta",
        "with_ast_tool_calls",
        "without_ast_tool_calls",
        "tool_calls_delta",
        "with_ast_ast_tool_calls",
        "with_ast_validation_success",
        "without_ast_validation_success",
    ]

    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()

        for c in comparisons:
            w = c.with_ast
            wo = c.without_ast

            w_elapsed = w.elapsed_seconds if w else None
            wo_elapsed = wo.elapsed_seconds if wo else None

            w_input = w.tokens.get("input") if w else None
            wo_input = wo.tokens.get("input") if wo else None

            w_output = w.tokens.get("output") if w else None
            wo_output = wo.tokens.get("output") if wo else None

            w_total = _total_tokens(w.tokens) if w else None
            wo_total = _total_tokens(wo.tokens) if wo else None

            w_tool_calls = sum(w.tools.values()) if w else None
            wo_tool_calls = sum(wo.tools.values()) if wo else None

            w_ast_calls = sum(w.ast_tool.values()) if w else None

            w_success = w.success if w else None
            wo_success = wo.success if wo else None

            success_changed = (
                w_success != wo_success
                if w_success is not None and wo_success is not None
                else None
            )

            w_val_success = (
                w.validation.get("success") if w and w.validation else None
            )
            wo_val_success = (
                wo.validation.get("success") if wo and wo.validation else None
            )

            writer.writerow({
                "task_id": c.task_id,
                "level": c.level,
                "with_ast_present": w is not None,
                "without_ast_present": wo is not None,
                "with_ast_success": w_success if w_success is not None else "",
                "without_ast_success": wo_success if wo_success is not None else "",
                "success_changed": success_changed if success_changed is not None else "",
                "with_ast_elapsed_seconds": fmt(w_elapsed),
                "without_ast_elapsed_seconds": fmt(wo_elapsed),
                "elapsed_delta_seconds": fmt(delta(w_elapsed, wo_elapsed)),
                "with_ast_input_tokens": w_input if w_input is not None else "",
                "without_ast_input_tokens": wo_input if wo_input is not None else "",
                "input_tokens_delta": (
                    w_input - wo_input
                    if w_input is not None and wo_input is not None
                    else ""
                ),
                "with_ast_output_tokens": w_output if w_output is not None else "",
                "without_ast_output_tokens": wo_output if wo_output is not None else "",
                "output_tokens_delta": (
                    w_output - wo_output
                    if w_output is not None and wo_output is not None
                    else ""
                ),
                "with_ast_total_tokens": w_total if w_total is not None else "",
                "without_ast_total_tokens": wo_total if wo_total is not None else "",
                "total_tokens_delta": (
                    w_total - wo_total
                    if w_total is not None and wo_total is not None
                    else ""
                ),
                "with_ast_tool_calls": w_tool_calls if w_tool_calls is not None else "",
                "without_ast_tool_calls": wo_tool_calls if wo_tool_calls is not None else "",
                "tool_calls_delta": (
                    w_tool_calls - wo_tool_calls
                    if w_tool_calls is not None and wo_tool_calls is not None
                    else ""
                ),
                "with_ast_ast_tool_calls": w_ast_calls if w_ast_calls is not None else "",
                "with_ast_validation_success": (
                    w_val_success if w_val_success is not None else ""
                ),
                "without_ast_validation_success": (
                    wo_val_success if wo_val_success is not None else ""
                ),
            })


# ---------------------------------------------------------------------------
# Unmatched tasks CSV
# ---------------------------------------------------------------------------

def write_unmatched_tasks_csv(
    comparisons: list[TaskComparison],
    path: Path,
) -> None:
    rows = unmatched(comparisons)

    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(
            f,
            fieldnames=["task_id", "level", "has_with_ast", "has_without_ast"],
        )
        writer.writeheader()

        for c in rows:
            writer.writerow({
                "task_id": c.task_id,
                "level": c.level,
                "has_with_ast": c.with_ast is not None,
                "has_without_ast": c.without_ast is not None,
            })


# ---------------------------------------------------------------------------
# Success comparison
# ---------------------------------------------------------------------------

def _success_rows(
    comparisons: list[TaskComparison],
) -> list[dict[str, Any]]:
    levels = _levels_in(comparisons)
    mc = matched(comparisons)

    def _row(label: str, group: list[TaskComparison]) -> dict[str, Any]:
        both_s = sum(
            1 for c in group
            if c.with_ast and c.without_ast
            and c.with_ast.success and c.without_ast.success
        )
        both_f = sum(
            1 for c in group
            if c.with_ast and c.without_ast
            and not c.with_ast.success and not c.without_ast.success
        )
        improved = sum(
            1 for c in group
            if c.with_ast and c.without_ast
            and c.with_ast.success and not c.without_ast.success
        )
        regressed = sum(
            1 for c in group
            if c.with_ast and c.without_ast
            and not c.with_ast.success and c.without_ast.success
        )
        total = len(group)

        with_success_count = sum(
            1 for c in group if c.with_ast and c.with_ast.success
        )
        wo_success_count = sum(
            1 for c in group if c.without_ast and c.without_ast.success
        )

        w_rate = with_success_count / total * 100.0 if total else None
        wo_rate = wo_success_count / total * 100.0 if total else None
        rate_delta = delta(w_rate, wo_rate)

        return {
            "level": label,
            "total_compared": total,
            "both_success": both_s,
            "both_failure": both_f,
            "improved_with_ast": improved,
            "regressed_with_ast": regressed,
            "with_ast_success_rate": fmt(w_rate, 1),
            "without_ast_success_rate": fmt(wo_rate, 1),
            "success_rate_delta": fmt(rate_delta, 1),
        }

    rows = [_row("overall", mc)]
    for lv in levels:
        group = [c for c in mc if c.level == lv]
        if group:
            rows.append(_row(lv, group))

    return rows


def write_success_comparison_csv(
    comparisons: list[TaskComparison],
    path: Path,
) -> None:
    rows = _success_rows(comparisons)

    fieldnames = [
        "level",
        "total_compared",
        "both_success",
        "both_failure",
        "improved_with_ast",
        "regressed_with_ast",
        "with_ast_success_rate",
        "without_ast_success_rate",
        "success_rate_delta",
    ]

    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


# ---------------------------------------------------------------------------
# Time comparison
# ---------------------------------------------------------------------------

def _time_rows(
    comparisons: list[TaskComparison],
) -> list[dict[str, Any]]:
    levels = _levels_in(comparisons)
    mc = matched(comparisons)

    def _row(label: str, group: list[TaskComparison]) -> dict[str, Any]:
        w_times = [
            c.with_ast.elapsed_seconds
            for c in group
            if c.with_ast and c.with_ast.elapsed_seconds is not None
        ]
        wo_times = [
            c.without_ast.elapsed_seconds
            for c in group
            if c.without_ast and c.without_ast.elapsed_seconds is not None
        ]

        w_avg = safe_mean(w_times)
        wo_avg = safe_mean(wo_times)
        w_med = safe_median(w_times)
        wo_med = safe_median(wo_times)

        return {
            "level": label,
            "matched_tasks": len(group),
            "with_ast_average_seconds": fmt(w_avg),
            "without_ast_average_seconds": fmt(wo_avg),
            "average_delta_seconds": fmt(delta(w_avg, wo_avg)),
            "with_ast_median_seconds": fmt(w_med),
            "without_ast_median_seconds": fmt(wo_med),
            "median_delta_seconds": fmt(delta(w_med, wo_med)),
            "with_ast_total_seconds": fmt(sum(w_times) if w_times else None),
            "without_ast_total_seconds": fmt(sum(wo_times) if wo_times else None),
            "total_delta_seconds": fmt(
                delta(
                    sum(w_times) if w_times else None,
                    sum(wo_times) if wo_times else None,
                )
            ),
        }

    rows = [_row("overall", mc)]
    for lv in levels:
        group = [c for c in mc if c.level == lv]
        if group:
            rows.append(_row(lv, group))

    return rows


def write_time_comparison_csv(
    comparisons: list[TaskComparison],
    path: Path,
) -> None:
    rows = _time_rows(comparisons)

    fieldnames = [
        "level",
        "matched_tasks",
        "with_ast_average_seconds",
        "without_ast_average_seconds",
        "average_delta_seconds",
        "with_ast_median_seconds",
        "without_ast_median_seconds",
        "median_delta_seconds",
        "with_ast_total_seconds",
        "without_ast_total_seconds",
        "total_delta_seconds",
    ]

    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


# ---------------------------------------------------------------------------
# Token comparison
# ---------------------------------------------------------------------------

def _token_rows(
    comparisons: list[TaskComparison],
) -> list[dict[str, Any]]:
    levels = _levels_in(comparisons)
    mc = matched(comparisons)

    token_keys = ("input", "output", "cache_read", "cache_creation", "total")

    def _collect(group: list[TaskComparison], side: str, key: str) -> list[int]:
        out: list[int] = []
        for c in group:
            r: NormalizedResult | None = getattr(c, side)
            if r is None:
                continue
            if key == "total":
                v = _total_tokens(r.tokens)
            else:
                v = r.tokens.get(key)
            if v is not None:
                out.append(v)
        return out

    def _row(label: str, group: list[TaskComparison]) -> dict[str, Any]:
        row: dict[str, Any] = {
            "level": label,
            "matched_tasks": len(group),
        }

        for key in token_keys:
            w_vals = _collect(group, "with_ast", key)
            wo_vals = _collect(group, "without_ast", key)

            w_avg = safe_mean([float(v) for v in w_vals])
            wo_avg = safe_mean([float(v) for v in wo_vals])

            row[f"with_ast_avg_{key}_tokens"] = fmt(w_avg, 0)
            row[f"without_ast_avg_{key}_tokens"] = fmt(wo_avg, 0)
            row[f"{key}_delta"] = fmt(delta(w_avg, wo_avg), 0)

        return row

    rows = [_row("overall", mc)]
    for lv in levels:
        group = [c for c in mc if c.level == lv]
        if group:
            rows.append(_row(lv, group))

    return rows


def write_token_comparison_csv(
    comparisons: list[TaskComparison],
    path: Path,
) -> None:
    token_keys = ("input", "output", "cache_read", "cache_creation", "total")

    fieldnames = ["level", "matched_tasks"]
    for key in token_keys:
        fieldnames += [
            f"with_ast_avg_{key}_tokens",
            f"without_ast_avg_{key}_tokens",
            f"{key}_delta",
        ]

    rows = _token_rows(comparisons)

    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


# ---------------------------------------------------------------------------
# Tool usage comparison
# ---------------------------------------------------------------------------

def _all_tools(comparisons: list[TaskComparison]) -> list[str]:
    names: set[str] = set()
    for c in comparisons:
        if c.with_ast:
            names.update(c.with_ast.tools)
        if c.without_ast:
            names.update(c.without_ast.tools)
    return sorted(names)


def _tool_rows(
    comparisons: list[TaskComparison],
) -> list[dict[str, Any]]:
    levels = _levels_in(comparisons)
    all_tool_names = _all_tools(comparisons)
    rows: list[dict[str, Any]] = []

    def _row(
        label: str,
        group: list[TaskComparison],
        tool: str,
    ) -> dict[str, Any]:
        w_total = sum(
            c.with_ast.tools.get(tool, 0)
            for c in group if c.with_ast
        )
        wo_total = sum(
            c.without_ast.tools.get(tool, 0)
            for c in group if c.without_ast
        )

        w_tasks = sum(1 for c in group if c.with_ast)
        wo_tasks = sum(1 for c in group if c.without_ast)

        w_avg = w_total / w_tasks if w_tasks else None
        wo_avg = wo_total / wo_tasks if wo_tasks else None

        return {
            "level": label,
            "tool": tool,
            "with_ast_total": w_total,
            "without_ast_total": wo_total,
            "total_delta": w_total - wo_total,
            "with_ast_avg_per_task": fmt(w_avg, 2),
            "without_ast_avg_per_task": fmt(wo_avg, 2),
            "avg_delta": fmt(delta(w_avg, wo_avg), 2),
        }

    for tool in all_tool_names:
        rows.append(_row("overall", comparisons, tool))
        for lv in levels:
            group = [c for c in comparisons if c.level == lv]
            if group:
                rows.append(_row(lv, group, tool))

    return rows


def write_tool_usage_comparison_csv(
    comparisons: list[TaskComparison],
    path: Path,
) -> None:
    rows = _tool_rows(comparisons)

    fieldnames = [
        "level",
        "tool",
        "with_ast_total",
        "without_ast_total",
        "total_delta",
        "with_ast_avg_per_task",
        "without_ast_avg_per_task",
        "avg_delta",
    ]

    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


# ---------------------------------------------------------------------------
# Tool call summary
# ---------------------------------------------------------------------------

def _tool_call_summary_rows(
    comparisons: list[TaskComparison],
) -> list[dict[str, Any]]:
    levels = _levels_in(comparisons)

    def _row(label: str, group: list[TaskComparison]) -> dict[str, Any]:
        w_calls = [
            sum(c.with_ast.tools.values())
            for c in group if c.with_ast
        ]
        wo_calls = [
            sum(c.without_ast.tools.values())
            for c in group if c.without_ast
        ]
        w_ast_calls = [
            sum(c.with_ast.ast_tool.values())
            for c in group if c.with_ast
        ]

        w_avg = safe_mean([float(v) for v in w_calls])
        wo_avg = safe_mean([float(v) for v in wo_calls])
        w_ast_avg = safe_mean([float(v) for v in w_ast_calls])

        return {
            "level": label,
            "with_ast_average_tool_calls": fmt(w_avg, 2),
            "without_ast_average_tool_calls": fmt(wo_avg, 2),
            "tool_calls_delta": fmt(delta(w_avg, wo_avg), 2),
            "with_ast_average_ast_tool_calls": fmt(w_ast_avg, 2),
        }

    rows = [_row("overall", comparisons)]
    for lv in levels:
        group = [c for c in comparisons if c.level == lv]
        if group:
            rows.append(_row(lv, group))

    return rows


def write_tool_call_summary_csv(
    comparisons: list[TaskComparison],
    path: Path,
) -> None:
    rows = _tool_call_summary_rows(comparisons)

    fieldnames = [
        "level",
        "with_ast_average_tool_calls",
        "without_ast_average_tool_calls",
        "tool_calls_delta",
        "with_ast_average_ast_tool_calls",
    ]

    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


# ---------------------------------------------------------------------------
# ast-tool command usage
# ---------------------------------------------------------------------------

def _all_ast_commands(
    with_ast: dict[str, NormalizedResult],
) -> list[str]:
    names: set[str] = set()
    for r in with_ast.values():
        names.update(r.ast_tool)
    return sorted(names)


def write_ast_tool_command_usage_csv(
    with_ast: dict[str, NormalizedResult],
    path: Path,
) -> None:
    commands = _all_ast_commands(with_ast)
    results = list(with_ast.values())

    levels_found: set[str] = {r.level for r in results}
    levels = [lv for lv in LEVEL_ORDER if lv in levels_found]
    levels += sorted(levels_found - set(LEVEL_ORDER))

    fieldnames = ["command", "total", *levels]

    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()

        for cmd in commands:
            total = sum(r.ast_tool.get(cmd, 0) for r in results)

            row: dict[str, Any] = {
                "command": cmd,
                "total": total,
            }
            for lv in levels:
                row[lv] = sum(
                    r.ast_tool.get(cmd, 0)
                    for r in results
                    if r.level == lv
                )

            writer.writerow(row)


def write_ast_tool_command_usage_per_task_csv(
    with_ast: dict[str, NormalizedResult],
    path: Path,
) -> None:
    commands = _all_ast_commands(with_ast)
    results = list(with_ast.values())

    levels_found: set[str] = {r.level for r in results}
    levels = [lv for lv in LEVEL_ORDER if lv in levels_found]
    levels += sorted(levels_found - set(LEVEL_ORDER))

    fieldnames = [
        "command",
        "level",
        "tasks_using_command",
        "total_calls",
        "average_calls_per_using_task",
    ]

    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()

        for cmd in commands:
            # Overall
            using = [r for r in results if r.ast_tool.get(cmd, 0) > 0]
            total_calls = sum(r.ast_tool.get(cmd, 0) for r in using)
            avg = total_calls / len(using) if using else None

            writer.writerow({
                "command": cmd,
                "level": "overall",
                "tasks_using_command": len(using),
                "total_calls": total_calls,
                "average_calls_per_using_task": fmt(avg, 2),
            })

            for lv in levels:
                lv_results = [r for r in results if r.level == lv]
                lv_using = [r for r in lv_results if r.ast_tool.get(cmd, 0) > 0]
                lv_total = sum(r.ast_tool.get(cmd, 0) for r in lv_using)
                lv_avg = lv_total / len(lv_using) if lv_using else None

                writer.writerow({
                    "command": cmd,
                    "level": lv,
                    "tasks_using_command": len(lv_using),
                    "total_calls": lv_total,
                    "average_calls_per_using_task": fmt(lv_avg, 2),
                })


# ---------------------------------------------------------------------------
# ast-tool success/failure analysis
# ---------------------------------------------------------------------------

def write_ast_tool_success_failure_csv(
    with_ast: dict[str, NormalizedResult],
    path: Path,
) -> None:
    commands = _all_ast_commands(with_ast)
    results = list(with_ast.values())

    fieldnames = [
        "command",
        "successful_tasks_using",
        "successful_total_calls",
        "successful_avg_calls",
        "failed_tasks_using",
        "failed_total_calls",
        "failed_avg_calls",
    ]

    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()

        for cmd in commands:
            success_using = [
                r for r in results
                if r.success and r.ast_tool.get(cmd, 0) > 0
            ]
            fail_using = [
                r for r in results
                if not r.success and r.ast_tool.get(cmd, 0) > 0
            ]

            s_total = sum(r.ast_tool.get(cmd, 0) for r in success_using)
            f_total = sum(r.ast_tool.get(cmd, 0) for r in fail_using)

            s_avg = s_total / len(success_using) if success_using else None
            f_avg = f_total / len(fail_using) if fail_using else None

            writer.writerow({
                "command": cmd,
                "successful_tasks_using": len(success_using),
                "successful_total_calls": s_total,
                "successful_avg_calls": fmt(s_avg, 2),
                "failed_tasks_using": len(fail_using),
                "failed_total_calls": f_total,
                "failed_avg_calls": fmt(f_avg, 2),
            })


# ---------------------------------------------------------------------------
# Per-task ast-tool intensity
# ---------------------------------------------------------------------------

def write_per_task_ast_tool_usage_csv(
    with_ast: dict[str, NormalizedResult],
    path: Path,
) -> None:
    commands = _all_ast_commands(with_ast)
    results = list(with_ast.values())

    fieldnames = [
        "task_id",
        "level",
        "success",
        "ast_tool_total_calls",
        *[f"{cmd}_calls" for cmd in commands],
    ]

    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()

        for r in sorted(results, key=lambda x: x.task_id):
            row: dict[str, Any] = {
                "task_id": r.task_id,
                "level": r.level,
                "success": r.success,
                "ast_tool_total_calls": sum(r.ast_tool.values()),
            }
            for cmd in commands:
                row[f"{cmd}_calls"] = r.ast_tool.get(cmd, 0)

            writer.writerow(row)


# ---------------------------------------------------------------------------
# Workflow comparison
# ---------------------------------------------------------------------------

def write_workflow_summary_csv(
    comparisons: list[TaskComparison],
    path: Path,
) -> None:
    levels = _levels_in(comparisons)

    def _row(label: str, group: list[TaskComparison]) -> dict[str, Any]:
        w_lens = [
            len(c.with_ast.workflow)
            for c in group if c.with_ast
        ]
        wo_lens = [
            len(c.without_ast.workflow)
            for c in group if c.without_ast
        ]

        w_avg = safe_mean([float(v) for v in w_lens])
        wo_avg = safe_mean([float(v) for v in wo_lens])

        return {
            "level": label,
            "with_ast_average_workflow_length": fmt(w_avg, 2),
            "without_ast_average_workflow_length": fmt(wo_avg, 2),
            "workflow_length_delta": fmt(delta(w_avg, wo_avg), 2),
        }

    rows = [_row("overall", comparisons)]
    for lv in levels:
        group = [c for c in comparisons if c.level == lv]
        if group:
            rows.append(_row(lv, group))

    fieldnames = [
        "level",
        "with_ast_average_workflow_length",
        "without_ast_average_workflow_length",
        "workflow_length_delta",
    ]

    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def write_first_ast_tool_command_csv(
    with_ast: dict[str, NormalizedResult],
    path: Path,
) -> None:
    fieldnames = ["task_id", "level", "first_ast_tool_command", "success"]

    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()

        for task_id, r in sorted(with_ast.items()):
            first_cmd: str | None = None

            for step in r.workflow:
                if isinstance(step, dict):
                    cmd = step.get("ast_tool_command")
                    if cmd:
                        first_cmd = str(cmd)
                        break

            writer.writerow({
                "task_id": task_id,
                "level": r.level,
                "first_ast_tool_command": first_cmd if first_cmd else "",
                "success": r.success,
            })


# ---------------------------------------------------------------------------
# Text summary
# ---------------------------------------------------------------------------

def build_summary_data(
    comparisons: list[TaskComparison],
    with_ast: dict[str, NormalizedResult],
    without_ast: dict[str, NormalizedResult],
) -> dict[str, Any]:
    mc = matched(comparisons)
    um = unmatched(comparisons)

    total_with = len(with_ast)
    total_without = len(without_ast)
    total_compared = len(mc)

    w_success = sum(1 for r in with_ast.values() if r.success)
    wo_success = sum(1 for r in without_ast.values() if r.success)

    w_times = [
        r.elapsed_seconds for r in with_ast.values()
        if r.elapsed_seconds is not None
    ]
    wo_times = [
        r.elapsed_seconds for r in without_ast.values()
        if r.elapsed_seconds is not None
    ]

    w_totals = [
        v for r in with_ast.values()
        for v in [_total_tokens(r.tokens)] if v is not None
    ]
    wo_totals = [
        v for r in without_ast.values()
        for v in [_total_tokens(r.tokens)] if v is not None
    ]

    w_tool_calls = [
        sum(r.tools.values()) for r in with_ast.values()
    ]
    wo_tool_calls = [
        sum(r.tools.values()) for r in without_ast.values()
    ]

    all_ast_commands: Counter[str] = Counter()
    for r in with_ast.values():
        all_ast_commands.update(r.ast_tool)

    all_tools = _all_tools(comparisons)
    tool_deltas: dict[str, dict[str, Any]] = {}
    for tool in all_tools:
        w_t = [r.tools.get(tool, 0) for r in with_ast.values()]
        wo_t = [r.tools.get(tool, 0) for r in without_ast.values()]
        tool_deltas[tool] = {
            "with_avg": safe_mean([float(v) for v in w_t]),
            "without_avg": safe_mean([float(v) for v in wo_t]),
        }

    success_rows = _success_rows(comparisons)
    overall_success = success_rows[0] if success_rows else {}

    return {
        "total_with_ast": total_with,
        "total_without_ast": total_without,
        "total_compared": total_compared,
        "unmatched_count": len(um),
        "with_ast_success": w_success,
        "without_ast_success": wo_success,
        "improved_with_ast": overall_success.get("improved_with_ast", 0),
        "regressed_with_ast": overall_success.get("regressed_with_ast", 0),
        "with_ast_avg_seconds": safe_mean(w_times),
        "without_ast_avg_seconds": safe_mean(wo_times),
        "with_ast_avg_tokens": safe_mean([float(v) for v in w_totals]),
        "without_ast_avg_tokens": safe_mean([float(v) for v in wo_totals]),
        "with_ast_avg_tool_calls": safe_mean([float(v) for v in w_tool_calls]),
        "without_ast_avg_tool_calls": safe_mean([float(v) for v in wo_tool_calls]),
        "ast_tool_total_calls": sum(all_ast_commands.values()),
        "ast_tool_commands": dict(all_ast_commands.most_common()),
        "tool_deltas": {
            tool: {
                "with_avg": d["with_avg"],
                "without_avg": d["without_avg"],
                "delta": delta(d["with_avg"], d["without_avg"]),
            }
            for tool, d in tool_deltas.items()
        },
    }


def write_summary_txt(
    data: dict[str, Any],
    path: Path,
    with_label: str,
    without_label: str,
) -> None:
    lines: list[str] = []

    def h(title: str) -> None:
        lines.append("")
        lines.append(title)
        lines.append("-" * len(title))

    lines.append("Evaluation Comparison")
    lines.append("=====================")
    lines.append("")
    lines.append(f"With ast-tool ({with_label}):    {data['total_with_ast']} tasks")
    lines.append(f"Without ast-tool ({without_label}): {data['total_without_ast']} tasks")
    lines.append(f"Matched (compared):          {data['total_compared']} tasks")
    lines.append(f"Unmatched:                   {data['unmatched_count']} tasks")

    h("Success")
    lines.append(
        f"With ast-tool:     {data['with_ast_success']} / {data['total_with_ast']}"
    )
    lines.append(
        f"Without ast-tool:  {data['without_ast_success']} / {data['total_without_ast']}"
    )
    lines.append(
        f"Improved with ast: {data['improved_with_ast']}"
    )
    lines.append(
        f"Regressed with ast:{data['regressed_with_ast']}"
    )

    h("Execution Time")
    w_avg = data.get("with_ast_avg_seconds")
    wo_avg = data.get("without_ast_avg_seconds")
    lines.append(
        f"With ast-tool average:    {fmt(w_avg, 2)} sec"
    )
    lines.append(
        f"Without ast-tool average: {fmt(wo_avg, 2)} sec"
    )
    d = delta(w_avg, wo_avg)
    sign = "+" if d is not None and d >= 0 else ""
    lines.append(
        f"Difference:               {sign}{fmt(d, 2)} sec"
    )

    h("Token Usage")
    w_tok = data.get("with_ast_avg_tokens")
    wo_tok = data.get("without_ast_avg_tokens")
    lines.append(
        f"With ast-tool average:    {fmt(w_tok, 0)}"
    )
    lines.append(
        f"Without ast-tool average: {fmt(wo_tok, 0)}"
    )
    td = delta(w_tok, wo_tok)
    sign = "+" if td is not None and td >= 0 else ""
    lines.append(
        f"Difference:               {sign}{fmt(td, 0)}"
    )

    h("Tool Calls")
    w_tc = data.get("with_ast_avg_tool_calls")
    wo_tc = data.get("without_ast_avg_tool_calls")
    lines.append(
        f"With ast-tool average:    {fmt(w_tc, 2)}"
    )
    lines.append(
        f"Without ast-tool average: {fmt(wo_tc, 2)}"
    )
    tc_d = delta(w_tc, wo_tc)
    sign = "+" if tc_d is not None and tc_d >= 0 else ""
    lines.append(
        f"Difference:               {sign}{fmt(tc_d, 2)}"
    )

    h("Existing Tool Changes")
    for tool, vals in sorted(data["tool_deltas"].items()):
        w_a = vals["with_avg"]
        wo_a = vals["without_avg"]
        d_v = vals["delta"]
        sign = "+" if d_v is not None and d_v >= 0 else ""
        lines.append(f"  {tool}:")
        lines.append(f"    with ast-tool:    {fmt(w_a, 2)} / task")
        lines.append(f"    without ast-tool: {fmt(wo_a, 2)} / task")
        lines.append(f"    delta:            {sign}{fmt(d_v, 2)}")

    h("ast-tool")
    lines.append(
        f"Total ast-tool calls: {data['ast_tool_total_calls']}"
    )
    lines.append("")
    lines.append("Most used commands:")
    lines.append("")
    for cmd, count in data["ast_tool_commands"].items():
        lines.append(f"  {cmd:<20} {count}")

    lines.append("")

    path.write_text("\n".join(lines), encoding="utf-8")


def write_summary_json(
    data: dict[str, Any],
    path: Path,
) -> None:
    serializable = dict(data)
    for key in ("with_ast_avg_seconds", "without_ast_avg_seconds",
                "with_ast_avg_tokens", "without_ast_avg_tokens",
                "with_ast_avg_tool_calls", "without_ast_avg_tool_calls"):
        v = serializable.get(key)
        if v is not None:
            serializable[key] = round(v, 4)

    for tool, vals in serializable.get("tool_deltas", {}).items():
        for k, v in vals.items():
            if isinstance(v, float):
                vals[k] = round(v, 4)

    path.write_text(
        json.dumps(serializable, indent=2, ensure_ascii=False),
        encoding="utf-8",
    )


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Compare two ast-tool evaluation result sets."
    )

    parser.add_argument(
        "--with-ast",
        required=True,
        type=Path,
        metavar="PATH",
        help="JSONL results file with ast-tool enabled",
    )

    parser.add_argument(
        "--without-ast",
        required=True,
        type=Path,
        metavar="PATH",
        help="JSONL results file without ast-tool",
    )

    parser.add_argument(
        "--output",
        type=Path,
        default=Path("comparison"),
        metavar="DIR",
        help="Output directory (default: comparison/)",
    )

    parser.add_argument(
        "--with-label",
        default="with_ast_tool",
        metavar="LABEL",
        help="Human-readable label for the with-ast condition",
    )

    parser.add_argument(
        "--without-label",
        default="without_ast_tool",
        metavar="LABEL",
        help="Human-readable label for the without-ast condition",
    )

    args = parser.parse_args()

    print(f"Loading {args.with_ast} ...")
    with_ast = load_normalized(args.with_ast)

    print(f"Loading {args.without_ast} ...")
    without_ast = load_normalized(args.without_ast)

    if not with_ast and not without_ast:
        print("ERROR: both result files are empty.")
        return 1

    print(
        f"With ast-tool: {len(with_ast)} tasks  |  "
        f"Without ast-tool: {len(without_ast)} tasks"
    )

    comparisons = build_task_comparisons(with_ast, without_ast)
    mc = matched(comparisons)
    um = unmatched(comparisons)

    print(f"Matched: {len(mc)}  |  Unmatched: {len(um)}")

    out = args.output
    out.mkdir(parents=True, exist_ok=True)

    write_per_task_csv(comparisons, out / "per_task.csv")
    print(f"Wrote {out / 'per_task.csv'}")

    write_unmatched_tasks_csv(comparisons, out / "unmatched_tasks.csv")
    print(f"Wrote {out / 'unmatched_tasks.csv'}")

    write_success_comparison_csv(comparisons, out / "success_comparison.csv")
    print(f"Wrote {out / 'success_comparison.csv'}")

    write_time_comparison_csv(comparisons, out / "time_comparison.csv")
    print(f"Wrote {out / 'time_comparison.csv'}")

    write_token_comparison_csv(comparisons, out / "token_comparison.csv")
    print(f"Wrote {out / 'token_comparison.csv'}")

    write_tool_usage_comparison_csv(comparisons, out / "tool_usage_comparison.csv")
    print(f"Wrote {out / 'tool_usage_comparison.csv'}")

    write_tool_call_summary_csv(comparisons, out / "tool_call_summary.csv")
    print(f"Wrote {out / 'tool_call_summary.csv'}")

    if with_ast:
        write_ast_tool_command_usage_csv(with_ast, out / "ast_tool_command_usage.csv")
        print(f"Wrote {out / 'ast_tool_command_usage.csv'}")

        write_ast_tool_command_usage_per_task_csv(
            with_ast, out / "ast_tool_command_usage_per_task.csv"
        )
        print(f"Wrote {out / 'ast_tool_command_usage_per_task.csv'}")

        write_ast_tool_success_failure_csv(
            with_ast, out / "ast_tool_success_failure.csv"
        )
        print(f"Wrote {out / 'ast_tool_success_failure.csv'}")

        write_per_task_ast_tool_usage_csv(
            with_ast, out / "per_task_ast_tool_usage.csv"
        )
        print(f"Wrote {out / 'per_task_ast_tool_usage.csv'}")

        write_first_ast_tool_command_csv(
            with_ast, out / "first_ast_tool_command.csv"
        )
        print(f"Wrote {out / 'first_ast_tool_command.csv'}")

    write_workflow_summary_csv(comparisons, out / "workflow_summary.csv")
    print(f"Wrote {out / 'workflow_summary.csv'}")

    summary_data = build_summary_data(comparisons, with_ast, without_ast)

    write_summary_txt(
        summary_data, out / "summary.txt",
        args.with_label, args.without_label,
    )
    print(f"Wrote {out / 'summary.txt'}")

    write_summary_json(summary_data, out / "summary.json")
    print(f"Wrote {out / 'summary.json'}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
