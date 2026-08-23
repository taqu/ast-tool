#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import json
import statistics
from collections import Counter, defaultdict
from pathlib import Path
from typing import Any


# ---------------------------------------------------------------------------
# Loading
# ---------------------------------------------------------------------------

def load_results(path: Path) -> list[dict[str, Any]]:
    results: list[dict[str, Any]] = []

    with path.open("r", encoding="utf-8") as f:
        for line_number, line in enumerate(f, start=1):
            line = line.strip()

            if not line:
                continue

            try:
                data = json.loads(line)
            except json.JSONDecodeError as e:
                print(
                    f"WARNING: invalid JSON at {path}:{line_number}: {e}"
                )
                continue

            if not isinstance(data, dict):
                print(
                    f"WARNING: expected object at "
                    f"{path}:{line_number}"
                )
                continue

            results.append(data)

    return results


# ---------------------------------------------------------------------------
# Task grouping
# ---------------------------------------------------------------------------

def get_level(task_id: str) -> str:
    """
    Convert:

        smoke-001   -> smoke
        level1-001  -> level1
        level2-004  -> level2

    """
    if "-" not in task_id:
        return "unknown"

    return task_id.split("-", 1)[0]


# ---------------------------------------------------------------------------
# Statistics helpers
# ---------------------------------------------------------------------------

def safe_mean(values: list[float]) -> float:
    if not values:
        return 0.0

    return statistics.mean(values)


def safe_median(values: list[float]) -> float:
    if not values:
        return 0.0

    return statistics.median(values)


def percentage(part: int, total: int) -> float:
    if total == 0:
        return 0.0

    return part * 100.0 / total


# ---------------------------------------------------------------------------
# Aggregation
# ---------------------------------------------------------------------------

def summarize_results(results: list[dict[str, Any]]) -> dict[str, Any]:

    summary: dict[str, Any] = {
        "total_tasks": 0,
        "success": 0,
        "failure": 0,
        "timeout": 0,

        "elapsed_seconds": [],
        "tokens": {
            "input": [],
            "output": [],
            "cache_read": [],
            "cache_creation": [],
            "total": [],
        },

        "tools": Counter(),
        "ast_tool": Counter(),

        "levels": defaultdict(
            lambda: {
                "tasks": 0,
                "success": 0,
                "failure": 0,
                "timeout": 0,

                "elapsed_seconds": [],

                "tokens": {
                    "input": [],
                    "output": [],
                    "cache_read": [],
                    "cache_creation": [],
                    "total": [],
                },

                "tools": Counter(),
                "ast_tool": Counter(),
            }
        ),
    }

    for result in results:

        task_id = str(result.get("task_id", "unknown"))
        level = get_level(task_id)

        success = bool(result.get("success", False))
        timed_out = bool(result.get("timed_out", False))

        elapsed = float(result.get("elapsed_seconds", 0.0))

        tokens = result.get("tokens", {}) or {}

        input_tokens = int(tokens.get("input", 0))
        output_tokens = int(tokens.get("output", 0))
        cache_read_tokens = int(tokens.get("cache_read", 0))
        cache_creation_tokens = int(
            tokens.get("cache_creation", 0)
        )

        total_tokens = (
            input_tokens
            + output_tokens
            + cache_read_tokens
            + cache_creation_tokens
        )

        tools = result.get("tools", {}) or {}
        ast_tool = result.get("ast_tool", {}) or {}

        # ---------------------------------------------------------------
        # Global
        # ---------------------------------------------------------------

        summary["total_tasks"] += 1

        if success:
            summary["success"] += 1
        else:
            summary["failure"] += 1

        if timed_out:
            summary["timeout"] += 1

        summary["elapsed_seconds"].append(elapsed)

        summary["tokens"]["input"].append(input_tokens)
        summary["tokens"]["output"].append(output_tokens)
        summary["tokens"]["cache_read"].append(cache_read_tokens)
        summary["tokens"]["cache_creation"].append(
            cache_creation_tokens
        )
        summary["tokens"]["total"].append(total_tokens)

        summary["tools"].update(tools)
        summary["ast_tool"].update(ast_tool)

        # ---------------------------------------------------------------
        # Per-level
        # ---------------------------------------------------------------

        level_stats = summary["levels"][level]

        level_stats["tasks"] += 1

        if success:
            level_stats["success"] += 1
        else:
            level_stats["failure"] += 1

        if timed_out:
            level_stats["timeout"] += 1

        level_stats["elapsed_seconds"].append(elapsed)

        level_stats["tokens"]["input"].append(input_tokens)
        level_stats["tokens"]["output"].append(output_tokens)
        level_stats["tokens"]["cache_read"].append(
            cache_read_tokens
        )
        level_stats["tokens"]["cache_creation"].append(
            cache_creation_tokens
        )
        level_stats["tokens"]["total"].append(total_tokens)

        level_stats["tools"].update(tools)
        level_stats["ast_tool"].update(ast_tool)

    return summary


# ---------------------------------------------------------------------------
# Printing
# ---------------------------------------------------------------------------

def print_global_summary(summary: dict[str, Any]) -> None:

    total = summary["total_tasks"]
    success = summary["success"]

    print()
    print("=" * 72)
    print("GLOBAL SUMMARY")
    print("=" * 72)

    print(f"Tasks:       {total}")
    print(
        f"Success:     {success} "
        f"({percentage(success, total):.1f}%)"
    )
    print(
        f"Failure:     {summary['failure']} "
        f"({percentage(summary['failure'], total):.1f}%)"
    )
    print(
        f"Timeout:     {summary['timeout']} "
        f"({percentage(summary['timeout'], total):.1f}%)"
    )

    elapsed = summary["elapsed_seconds"]

    print()
    print("Elapsed time:")

    print(f"  Total:     {sum(elapsed):.2f}s")
    print(f"  Mean:      {safe_mean(elapsed):.2f}s")
    print(f"  Median:    {safe_median(elapsed):.2f}s")

    tokens = summary["tokens"]

    print()
    print("Token usage:")

    for key in (
        "input",
        "output",
        "cache_read",
        "cache_creation",
        "total",
    ):
        values = tokens[key]

        print(
            f"  {key:16}"
            f"total={sum(values):8d} "
            f"mean={safe_mean(values):10.1f}"
        )


def print_level_summary(summary: dict[str, Any]) -> None:

    print()
    print("=" * 72)
    print("PER-LEVEL SUMMARY")
    print("=" * 72)

    header = (
        f"{'Level':<10}"
        f"{'Tasks':>7}"
        f"{'Success':>12}"
        f"{'Avg Time':>12}"
        f"{'Avg Tokens':>14}"
    )

    print(header)
    print("-" * len(header))

    levels = summary["levels"]

    for level in sorted(levels.keys()):

        stats = levels[level]

        tasks = stats["tasks"]
        success = stats["success"]

        avg_time = safe_mean(
            stats["elapsed_seconds"]
        )

        avg_tokens = safe_mean(
            stats["tokens"]["total"]
        )

        print(
            f"{level:<10}"
            f"{tasks:>7}"
            f"{success:>5}/{tasks:<6}"
            f"{avg_time:>11.2f}s"
            f"{avg_tokens:>14.1f}"
        )


def print_tool_usage(summary: dict[str, Any]) -> None:

    print()
    print("=" * 72)
    print("TOOL USAGE")
    print("=" * 72)

    for name, count in summary["tools"].most_common():

        print(
            f"{name:<20} {count:>8}"
        )


def print_ast_tool_usage(summary: dict[str, Any]) -> None:

    print()
    print("=" * 72)
    print("AST-TOOL USAGE")
    print("=" * 72)

    if not summary["ast_tool"]:
        print("No ast-tool commands were used.")
        return

    total = sum(summary["ast_tool"].values())

    for command, count in summary["ast_tool"].most_common():

        print(
            f"{command:<20} "
            f"{count:>8} "
            f"({percentage(count, total):5.1f}%)"
        )


def print_ast_tool_by_level(
    summary: dict[str, Any]
) -> None:

    print()
    print("=" * 72)
    print("AST-TOOL USAGE BY LEVEL")
    print("=" * 72)

    for level in sorted(summary["levels"].keys()):

        stats = summary["levels"][level]

        print()
        print(level)

        if not stats["ast_tool"]:
            print("  (no ast-tool usage)")
            continue

        total = sum(stats["ast_tool"].values())

        for command, count in stats["ast_tool"].most_common():

            print(
                f"  {command:<18}"
                f"{count:>6} "
                f"({percentage(count, total):5.1f}%)"
            )


# ---------------------------------------------------------------------------
# Per-task CSV
# ---------------------------------------------------------------------------

def write_task_csv(
    results: list[dict[str, Any]],
    path: Path,
) -> None:

    fieldnames = [
        "task_id",
        "level",
        "success",
        "timed_out",
        "elapsed_seconds",

        "input_tokens",
        "output_tokens",
        "cache_read_tokens",
        "cache_creation_tokens",
        "total_tokens",

        "tool_calls",
        "ast_tool_calls",

        "changed_files",
        "process_exit_code",
    ]

    with path.open(
        "w",
        newline="",
        encoding="utf-8",
    ) as f:

        writer = csv.DictWriter(
            f,
            fieldnames=fieldnames,
        )

        writer.writeheader()

        for result in results:

            task_id = str(
                result.get("task_id", "unknown")
            )

            tokens = result.get("tokens", {}) or {}

            input_tokens = int(
                tokens.get("input", 0)
            )

            output_tokens = int(
                tokens.get("output", 0)
            )

            cache_read_tokens = int(
                tokens.get("cache_read", 0)
            )

            cache_creation_tokens = int(
                tokens.get("cache_creation", 0)
            )

            total_tokens = (
                input_tokens
                + output_tokens
                + cache_read_tokens
                + cache_creation_tokens
            )

            tools = result.get("tools", {}) or {}
            ast_tool = result.get("ast_tool", {}) or {}

            changed_files = (
                result.get("changed_files", []) or []
            )

            writer.writerow(
                {
                    "task_id": task_id,
                    "level": get_level(task_id),

                    "success": result.get(
                        "success",
                        False,
                    ),

                    "timed_out": result.get(
                        "timed_out",
                        False,
                    ),

                    "elapsed_seconds": result.get(
                        "elapsed_seconds",
                        0.0,
                    ),

                    "input_tokens": input_tokens,
                    "output_tokens": output_tokens,
                    "cache_read_tokens": cache_read_tokens,
                    "cache_creation_tokens": (
                        cache_creation_tokens
                    ),
                    "total_tokens": total_tokens,

                    "tool_calls": sum(
                        tools.values()
                    ),

                    "ast_tool_calls": sum(
                        ast_tool.values()
                    ),

                    "changed_files": ",".join(
                        changed_files
                    ),

                    "process_exit_code": result.get(
                        "process_exit_code",
                        "",
                    ),
                }
            )


# ---------------------------------------------------------------------------
# Per-command CSV
# ---------------------------------------------------------------------------

def write_ast_tool_csv(
    summary: dict[str, Any],
    path: Path,
) -> None:

    commands = set(
        summary["ast_tool"].keys()
    )

    for stats in summary["levels"].values():
        commands.update(
            stats["ast_tool"].keys()
        )

    levels = sorted(
        summary["levels"].keys()
    )

    fieldnames = [
        "command",
        "total",
        *levels,
    ]

    with path.open(
        "w",
        newline="",
        encoding="utf-8",
    ) as f:

        writer = csv.DictWriter(
            f,
            fieldnames=fieldnames,
        )

        writer.writeheader()

        for command in sorted(commands):

            row: dict[str, Any] = {
                "command": command,
                "total": summary["ast_tool"].get(
                    command,
                    0,
                ),
            }

            for level in levels:

                row[level] = (
                    summary["levels"][level]
                    ["ast_tool"]
                    .get(command, 0)
                )

            writer.writerow(row)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main() -> int:

    parser = argparse.ArgumentParser(
        description=(
            "Analyze ast-tool agent evaluation results."
        )
    )

    parser.add_argument(
        "results",
        type=Path,
        help="JSONL result file",
    )

    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path(
            "evaluation/statistics"
        ),
        help="Directory for CSV output",
    )

    args = parser.parse_args()

    results = load_results(
        args.results
    )

    if not results:
        print("No results found.")
        return 1

    summary = summarize_results(
        results
    )

    print_global_summary(summary)

    print_level_summary(summary)

    print_tool_usage(summary)

    print_ast_tool_usage(summary)

    print_ast_tool_by_level(summary)

    args.output_dir.mkdir(
        parents=True,
        exist_ok=True,
    )

    task_csv = (
        args.output_dir
        / "task_summary.csv"
    )

    ast_tool_csv = (
        args.output_dir
        / "ast_tool_usage.csv"
    )

    write_task_csv(
        results,
        task_csv,
    )

    write_ast_tool_csv(
        summary,
        ast_tool_csv,
    )

    print()
    print("=" * 72)
    print("OUTPUT")
    print("=" * 72)

    print(
        f"Task summary:    {task_csv}"
    )

    print(
        f"ast-tool usage:  {ast_tool_csv}"
    )

    return 0

if __name__ == "__main__":
    raise SystemExit(main())
