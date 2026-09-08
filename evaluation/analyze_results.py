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
# Token extraction
# ---------------------------------------------------------------------------

def extract_token_usage(
    result: dict[str, Any],
) -> dict[str, int | None]:
    """
    Extract token usage from a result dict.

    Canonical token source: result["tokens"] dict produced by the
    evaluation runner.  Total includes cache_read and cache_creation
    alongside input and output (preserving the original accounting).

    Returns None for any field that is absent, non-numeric, or negative
    so that missing values never corrupt aggregate averages.
    """
    raw = result.get("tokens")

    if not isinstance(raw, dict):
        return {
            "input": None,
            "output": None,
            "cache_read": None,
            "cache_creation": None,
            "total": None,
        }

    def parse(key: str) -> int | None:
        val = raw.get(key)

        if val is None:
            return None

        try:
            v = int(val)
        except (TypeError, ValueError):
            print(
                f"WARNING: invalid token value for '{key}': {val!r}"
            )
            return None

        if v < 0:
            print(
                f"WARNING: negative token value for '{key}': {v}"
            )
            return None

        return v

    inp = parse("input")
    out = parse("output")
    cr = parse("cache_read")
    cc = parse("cache_creation")

    parts = [x for x in (inp, out, cr, cc) if x is not None]
    total = sum(parts) if parts else None

    return {
        "input": inp,
        "output": out,
        "cache_read": cr,
        "cache_creation": cc,
        "total": total,
    }


def _empty_token_lists() -> dict[str, list[int]]:
    return {
        "input": [],
        "output": [],
        "cache_read": [],
        "cache_creation": [],
        "total": [],
    }


def _append_token_usage(
    bucket: dict[str, list[int]],
    usage: dict[str, int | None],
) -> None:
    for key in (
        "input",
        "output",
        "cache_read",
        "cache_creation",
        "total",
    ):
        val = usage.get(key)
        if val is not None:
            bucket[key].append(val)


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

        "tokens_with_data": 0,
        "tokens": _empty_token_lists(),

        "success_tokens": _empty_token_lists(),
        "failure_tokens": _empty_token_lists(),

        "ast_tool_used_count": 0,
        "ast_tool_not_used_count": 0,
        "ast_tool_used_tokens": _empty_token_lists(),
        "ast_tool_not_used_tokens": _empty_token_lists(),

        "tools": Counter(),
        "ast_tool": Counter(),

        "levels": defaultdict(
            lambda: {
                "tasks": 0,
                "success": 0,
                "failure": 0,
                "timeout": 0,

                "elapsed_seconds": [],

                "tokens_with_data": 0,
                "tokens": _empty_token_lists(),

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

        token_usage = extract_token_usage(result)
        has_tokens = isinstance(result.get("tokens"), dict)

        tools = result.get("tools", {}) or {}
        ast_tool = result.get("ast_tool", {}) or {}
        uses_ast_tool = bool(ast_tool)

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

        if has_tokens:
            summary["tokens_with_data"] += 1

        _append_token_usage(summary["tokens"], token_usage)

        if success:
            _append_token_usage(
                summary["success_tokens"], token_usage
            )
        else:
            _append_token_usage(
                summary["failure_tokens"], token_usage
            )

        if uses_ast_tool:
            summary["ast_tool_used_count"] += 1
            _append_token_usage(
                summary["ast_tool_used_tokens"], token_usage
            )
        else:
            summary["ast_tool_not_used_count"] += 1
            _append_token_usage(
                summary["ast_tool_not_used_tokens"], token_usage
            )

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

        if has_tokens:
            level_stats["tokens_with_data"] += 1

        _append_token_usage(level_stats["tokens"], token_usage)

        level_stats["tools"].update(tools)
        level_stats["ast_tool"].update(ast_tool)

    return summary


# ---------------------------------------------------------------------------
# Printing
# ---------------------------------------------------------------------------

def _fmt_tokens(values: list[int]) -> str:
    if not values:
        return "n/a"
    return (
        f"total={sum(values):10,d}  "
        f"mean={safe_mean(values):9,.1f}  "
        f"median={safe_median(values):9,.1f}  "
        f"min={min(values):9,d}  "
        f"max={max(values):9,d}"
    )


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

    tokens_with_data = summary["tokens_with_data"]
    tokens = summary["tokens"]

    print()
    print("Token usage:")
    print(
        f"  Data coverage: "
        f"{tokens_with_data}/{total} "
        f"({percentage(tokens_with_data, total):.1f}%)"
    )
    print()

    for key in (
        "input",
        "output",
        "cache_read",
        "cache_creation",
        "total",
    ):
        values = tokens[key]
        print(f"  {key:16} {_fmt_tokens(values)}")


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


def print_token_by_level(summary: dict[str, Any]) -> None:

    print()
    print("=" * 72)
    print("TOKEN USAGE BY LEVEL")
    print("=" * 72)

    levels = summary["levels"]

    for level in sorted(levels.keys()):

        stats = levels[level]
        tokens = stats["tokens"]
        tasks = stats["tasks"]
        with_data = stats["tokens_with_data"]

        total_vals = tokens["total"]

        print()
        print(
            f"{level}  "
            f"({tasks} tests, "
            f"token data: {with_data}/{tasks})"
        )

        if not total_vals:
            print("  no token data")
            continue

        print(
            f"  avg input:         "
            f"{safe_mean(tokens['input']):>10,.1f}"
        )
        print(
            f"  avg output:        "
            f"{safe_mean(tokens['output']):>10,.1f}"
        )
        print(
            f"  avg total:         "
            f"{safe_mean(total_vals):>10,.1f}"
        )
        print(
            f"  median total:      "
            f"{safe_median(total_vals):>10,.1f}"
        )


def print_token_by_outcome(summary: dict[str, Any]) -> None:

    print()
    print("=" * 72)
    print("TOKEN USAGE BY OUTCOME")
    print("=" * 72)

    groups = [
        ("Success", summary["success_tokens"], summary["success"]),
        ("Failure", summary["failure_tokens"], summary["failure"]),
    ]

    for label, token_dict, count in groups:

        total_vals = token_dict["total"]
        input_vals = token_dict["input"]
        output_vals = token_dict["output"]

        print()
        print(f"{label} ({count} tests):")

        if not total_vals:
            print("  no token data")
            continue

        print(
            f"  avg input tokens:  "
            f"{safe_mean(input_vals):>10,.1f}"
        )
        print(
            f"  avg output tokens: "
            f"{safe_mean(output_vals):>10,.1f}"
        )
        print(
            f"  avg total tokens:  "
            f"{safe_mean(total_vals):>10,.1f}"
        )
        print(
            f"  total tokens:      "
            f"{sum(total_vals):>10,d}"
        )


def print_ast_tool_token_comparison(
    summary: dict[str, Any],
) -> None:

    print()
    print("=" * 72)
    print("TOKEN USAGE: AST-TOOL vs NO AST-TOOL")
    print("=" * 72)

    groups = [
        (
            "With ast-tool",
            summary["ast_tool_used_tokens"],
            summary["ast_tool_used_count"],
            summary["ast_tool_used_count"],
        ),
        (
            "Without ast-tool",
            summary["ast_tool_not_used_tokens"],
            summary["ast_tool_not_used_count"],
            summary["ast_tool_not_used_count"],
        ),
    ]

    for label, token_dict, count, _ in groups:

        total_vals = token_dict["total"]
        input_vals = token_dict["input"]
        output_vals = token_dict["output"]

        tools_total = sum(
            summary["tools"].values()
        )

        print()
        print(f"{label} ({count} tests):")

        if not total_vals:
            print("  no token data")
            continue

        avg_total = safe_mean(total_vals)
        print(
            f"  avg input tokens:  "
            f"{safe_mean(input_vals):>10,.1f}"
        )
        print(
            f"  avg output tokens: "
            f"{safe_mean(output_vals):>10,.1f}"
        )
        print(
            f"  avg total tokens:  "
            f"{avg_total:>10,.1f}"
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
# Trace loading and AST-tool output measurement
# ---------------------------------------------------------------------------

def load_trace(path: Path) -> list[dict[str, Any]]:
    events: list[dict[str, Any]] = []

    with path.open("r", encoding="utf-8") as f:
        for line_number, line in enumerate(f, start=1):
            line = line.strip()
            if not line:
                continue
            try:
                data = json.loads(line)
            except json.JSONDecodeError as e:
                print(
                    f"WARNING: invalid JSON in trace {path}:{line_number}: {e}"
                )
                continue
            if isinstance(data, dict):
                events.append(data)

    return events


def _ast_tool_output_stats(
    traces_dir: Path,
    results: list[dict[str, Any]],
) -> dict[str, Any]:
    """
    Read per-task trace JSONL files and aggregate ast-tool output statistics.

    For each ast-tool tool call the trace records the full output text, the
    raw command line, and a detected-command name.  These are used to compute
    output byte counts and to classify calls as plain / JSON / pretty-JSON.

    Trace files are matched to result entries by task_id.
    """
    stats: dict[str, Any] = {
        "total_calls": 0,
        "total_bytes": 0,
        "plain_calls": 0,
        "json_calls": 0,
        "pretty_json_calls": 0,
        "by_command": defaultdict(
            lambda: {
                "calls": 0,
                "bytes": 0,
                "plain": 0,
                "json": 0,
                "pretty_json": 0,
            }
        ),
    }

    for result in results:
        task_id = str(result.get("task_id", "unknown"))

        trace_file = traces_dir / f"{task_id}.jsonl"

        if not trace_file.exists():
            continue

        events = load_trace(trace_file)

        for event in events:
            if event.get("event") != "tool_call":
                continue

            ast_info = event.get("ast_tool", {})
            if not ast_info.get("detected"):
                continue

            command = str(ast_info.get("command", "unknown"))
            output = str(event.get("output", ""))
            raw_cmd = str(ast_info.get("raw_command", ""))

            byte_count = len(output.encode("utf-8"))

            is_json = "--json" in raw_cmd
            is_pretty = is_json and "--pretty" in raw_cmd

            stats["total_calls"] += 1
            stats["total_bytes"] += byte_count

            cmd_stats = stats["by_command"][command]
            cmd_stats["calls"] += 1
            cmd_stats["bytes"] += byte_count

            if is_pretty:
                stats["pretty_json_calls"] += 1
                cmd_stats["pretty_json"] += 1
            elif is_json:
                stats["json_calls"] += 1
                cmd_stats["json"] += 1
            else:
                stats["plain_calls"] += 1
                cmd_stats["plain"] += 1

    return stats


def print_ast_tool_output_stats(
    output_stats: dict[str, Any],
) -> None:

    print()
    print("=" * 72)
    print("AST-TOOL OUTPUT VOLUME")
    print("=" * 72)

    total_calls = output_stats["total_calls"]

    if total_calls == 0:
        print("No trace data available.")
        return

    total_bytes = output_stats["total_bytes"]
    plain = output_stats["plain_calls"]
    json_calls = output_stats["json_calls"]
    pretty = output_stats["pretty_json_calls"]

    print(
        f"Total calls:         {total_calls:>8,}"
    )
    print(
        f"Total output bytes:  {total_bytes:>8,}"
    )
    if total_calls > 0:
        print(
            f"Avg bytes/call:      {total_bytes / total_calls:>8,.1f}"
        )
    print()
    print(
        f"Plain-text calls:    {plain:>8,} "
        f"({percentage(plain, total_calls):5.1f}%)"
    )
    print(
        f"JSON calls:          {json_calls:>8,} "
        f"({percentage(json_calls, total_calls):5.1f}%)"
    )
    print(
        f"Pretty-JSON calls:   {pretty:>8,} "
        f"({percentage(pretty, total_calls):5.1f}%)"
    )

    by_command = output_stats["by_command"]

    if not by_command:
        return

    print()
    header = (
        f"{'Command':<20}"
        f"{'Calls':>7}"
        f"{'Bytes':>10}"
        f"{'Avg B/call':>12}"
        f"{'Plain':>8}"
        f"{'JSON':>8}"
        f"{'Pretty':>8}"
    )
    print(header)
    print("-" * len(header))

    for command, cs in sorted(
        by_command.items(),
        key=lambda x: -x[1]["bytes"],
    ):
        avg = cs["bytes"] / cs["calls"] if cs["calls"] else 0.0

        print(
            f"{command:<20}"
            f"{cs['calls']:>7,}"
            f"{cs['bytes']:>10,}"
            f"{avg:>12,.1f}"
            f"{cs['plain']:>8}"
            f"{cs['json']:>8}"
            f"{cs['pretty_json']:>8}"
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
        "tokens_per_tool_call",

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

            token_usage = extract_token_usage(result)

            def fmt(val: int | None) -> str | int:
                return "" if val is None else val

            tools = result.get("tools", {}) or {}
            ast_tool = result.get("ast_tool", {}) or {}
            total_tool_calls = sum(tools.values())

            total_tokens = token_usage["total"]
            if (
                total_tokens is not None
                and total_tool_calls > 0
            ):
                tokens_per_call: str | float = (
                    total_tokens / total_tool_calls
                )
            else:
                tokens_per_call = ""

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

                    "input_tokens": fmt(
                        token_usage["input"]
                    ),
                    "output_tokens": fmt(
                        token_usage["output"]
                    ),
                    "cache_read_tokens": fmt(
                        token_usage["cache_read"]
                    ),
                    "cache_creation_tokens": fmt(
                        token_usage["cache_creation"]
                    ),
                    "total_tokens": fmt(
                        token_usage["total"]
                    ),

                    "tool_calls": total_tool_calls,

                    "ast_tool_calls": sum(
                        ast_tool.values()
                    ),

                    "tokens_per_tool_call": tokens_per_call,

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

    parser.add_argument(
        "--traces-dir",
        type=Path,
        default=None,
        help=(
            "Directory containing per-task trace JSONL files "
            "(task_id.jsonl).  When provided, ast-tool output "
            "volume statistics are included in the report."
        ),
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

    print_token_by_level(summary)

    print_token_by_outcome(summary)

    print_ast_tool_token_comparison(summary)

    print_tool_usage(summary)

    print_ast_tool_usage(summary)

    print_ast_tool_by_level(summary)

    if args.traces_dir is not None:
        output_stats = _ast_tool_output_stats(
            args.traces_dir, results
        )
        print_ast_tool_output_stats(output_stats)

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
