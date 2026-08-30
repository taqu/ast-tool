#!/usr/bin/env python3
"""Phase 0 baseline trace analyzer for ast-tool evaluation traces."""
from __future__ import annotations

import argparse
import json
import re
import statistics
from collections import defaultdict
from pathlib import Path
from typing import Any

# ---------------------------------------------------------------------------
# Regex patterns
# ---------------------------------------------------------------------------

_LEVEL_RE = re.compile(r'^level(\d+)-')
_AST_TOOL_CMD_RE = re.compile(r'\bast-tool\s+(\w[\w-]*)(.*)', re.DOTALL)
_AST_TOOL_TOPLEVEL_HELP_RE = re.compile(r'\bast-tool\s+--help\b')
_HELP_FLAG_RE = re.compile(r'(^|\s)--help\b')
_JSON_FLAG_RE = re.compile(r'(^|\s)--json\b')
_PRETTY_FLAG_RE = re.compile(r'(^|\s)--pretty\b')


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _parse_task_level(task_id: str) -> int | None:
    m = _LEVEL_RE.match(task_id)
    if m:
        return int(m.group(1))
    return None


def _get_ast_meta(event: dict[str, Any]) -> dict[str, Any] | None:
    """
    Return ast-tool metadata for a tool_call event.

    Prefers already-annotated ast_tool field from the tracer.
    Falls back to parsing the Bash command for top-level --help
    which the tracer's \\w+ regex misses.
    """
    if 'ast_tool' in event:
        return event['ast_tool']

    if event.get('tool') != 'Bash':
        return None

    inp = event.get('input', {})
    cmd = inp.get('command', '') if isinstance(inp, dict) else str(inp)

    # Top-level: ast-tool --help
    if _AST_TOOL_TOPLEVEL_HELP_RE.search(cmd):
        return {'detected': True, 'command': 'top_level', 'raw_command': cmd}

    m = _AST_TOOL_CMD_RE.search(cmd)
    if not m:
        return None
    return {'detected': True, 'command': m.group(1), 'raw_command': cmd}


def _is_help_invocation(raw_command: str) -> bool:
    return bool(_HELP_FLAG_RE.search(raw_command))


def _is_json_invocation(raw_command: str) -> bool:
    return bool(_JSON_FLAG_RE.search(raw_command))


def _is_pretty_invocation(raw_command: str) -> bool:
    return bool(_PRETTY_FLAG_RE.search(raw_command))


# ---------------------------------------------------------------------------
# Single-trace analysis
# ---------------------------------------------------------------------------

def analyze_trace(
    trace_path: Path,
    result_record: dict[str, Any] | None = None,
) -> dict[str, Any]:
    """
    Analyze a single JSONL trace file and return a metrics summary dict.

    result_record, if supplied, is the corresponding entry from results.jsonl
    and is used only to attach token metrics.
    """
    tool_call_events: list[dict[str, Any]] = []
    task_start: dict[str, Any] = {}
    task_end: dict[str, Any] = {}

    with trace_path.open('r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                ev = json.loads(line)
            except json.JSONDecodeError:
                continue
            etype = ev.get('event')
            if etype == 'task_start':
                task_start = ev
            elif etype == 'task_end':
                task_end = ev
            elif etype == 'tool_call':
                tool_call_events.append(ev)

    task_id = (
        task_start.get('task_id')
        or task_end.get('task_id')
        or trace_path.stem
    )
    task_level = _parse_task_level(task_id)
    agent = task_start.get('agent', 'unknown')
    repository = task_start.get('repository', '')
    success = bool(task_end.get('success', False))
    validation_success = bool(task_end.get('validation_success', False))
    elapsed_seconds = float(task_end.get('elapsed_seconds', 0.0))

    # ---- Per-event processing ----

    tool_counts: dict[str, int] = defaultdict(int)
    total_tool_calls = 0

    # AST tool aggregates
    ast_tool_calls = 0
    ast_tool_successes = 0
    ast_tool_failures = 0
    ast_tool_help_calls = 0
    ast_tool_help_by_command: dict[str, int] = defaultdict(int)
    ast_tool_json_calls = 0
    ast_tool_pretty_json_calls = 0
    ast_tool_commands: dict[str, int] = defaultdict(int)
    ast_tool_failures_by_command: dict[str, int] = defaultdict(int)

    tool_sequence: list[str] = []
    ast_tool_sequence: list[str] = []

    # Enriched per-event info for retry/recovery analysis
    ast_events: list[dict[str, Any]] = []   # {command, success, is_help, idx}
    enriched: list[dict[str, Any]] = []     # {is_ast, is_help, success} for every event

    for event in tool_call_events:
        tool_name = event.get('tool', 'Unknown')
        ok = bool(event.get('success', True))
        total_tool_calls += 1
        tool_counts[tool_name] += 1

        ast_meta = _get_ast_meta(event)

        if ast_meta:
            command = ast_meta.get('command', 'unknown')
            raw_cmd = ast_meta.get('raw_command', '')

            is_help = _is_help_invocation(raw_cmd) or command == 'top_level'
            is_json = _is_json_invocation(raw_cmd)
            is_pretty = is_json and _is_pretty_invocation(raw_cmd)

            ast_tool_calls += 1
            ast_tool_commands[command] += 1

            if is_help:
                ast_tool_help_calls += 1
                ast_tool_help_by_command[command] += 1

            if is_json:
                ast_tool_json_calls += 1
            if is_pretty:
                ast_tool_pretty_json_calls += 1

            if ok:
                ast_tool_successes += 1
            else:
                ast_tool_failures += 1
                ast_tool_failures_by_command[command] += 1

            tool_sequence.append(f'ast-tool:{command}')
            ast_tool_sequence.append(command)

            event_idx = len(enriched)
            ast_events.append({
                'command': command,
                'success': ok,
                'is_help': is_help,
                'idx': event_idx,
            })
            enriched.append({'is_ast': True, 'is_help': is_help, 'success': ok})
        else:
            tool_sequence.append(tool_name)
            enriched.append({'is_ast': False, 'is_help': False, 'success': ok})

    # ---- Retry detection ----
    # A retry: same subcommand invoked again after at least one prior failure of that command.
    # Once a command has failed, each subsequent invocation counts as a retry.
    ast_tool_retries = 0
    consecutive_ast_tool_retries = 0
    failed_commands: set[str] = set()
    prev_failed_command: str | None = None

    for ae in ast_events:
        cmd = ae['command']
        ok = ae['success']

        if cmd in failed_commands:
            ast_tool_retries += 1

        if not ok:
            failed_commands.add(cmd)
            if prev_failed_command == cmd:
                consecutive_ast_tool_retries += 1
            prev_failed_command = cmd
        else:
            prev_failed_command = None

    # ---- Recovery distance ----
    # For each failed ast-tool invocation, count tool events (any type) from that
    # failure (exclusive) to the next successful non-help ast-tool call (inclusive).
    ast_tool_recovery_distances: list[int] = []

    for ae in ast_events:
        if ae['success']:
            continue

        start_idx = ae['idx']
        distance = 0
        found = False
        for j in range(start_idx + 1, len(enriched)):
            distance += 1
            ev_j = enriched[j]
            if ev_j['is_ast'] and ev_j['success'] and not ev_j['is_help']:
                ast_tool_recovery_distances.append(distance)
                found = True
                break

    avg_recovery: float | None = (
        statistics.mean(ast_tool_recovery_distances)
        if ast_tool_recovery_distances else None
    )
    max_recovery: int | None = (
        max(ast_tool_recovery_distances)
        if ast_tool_recovery_distances else None
    )

    # ---- Assemble result ----

    result: dict[str, Any] = {
        'task_id': task_id,
        'task_level': task_level,
        'agent': agent,
        'repository': repository,
        'success': success,
        'validation_success': validation_success,
        'elapsed_seconds': elapsed_seconds,

        'total_tool_calls': total_tool_calls,
        'tools': dict(tool_counts),

        'bash_calls': tool_counts.get('Bash', 0),
        'read_calls': tool_counts.get('Read', 0),
        'edit_calls': tool_counts.get('Edit', 0),
        'grep_calls': tool_counts.get('Grep', 0),
        'glob_calls': tool_counts.get('Glob', 0),
        'skill_calls': tool_counts.get('Skill', 0),

        'ast_tool_calls': ast_tool_calls,
        'ast_tool_successes': ast_tool_successes,
        'ast_tool_failures': ast_tool_failures,
        'ast_tool_retries': ast_tool_retries,
        'ast_tool_help_calls': ast_tool_help_calls,
        'ast_tool_json_calls': ast_tool_json_calls,
        'ast_tool_pretty_json_calls': ast_tool_pretty_json_calls,

        'ast_tool_commands': dict(ast_tool_commands),
        'ast_tool_failures_by_command': dict(ast_tool_failures_by_command),

        'tool_sequence': tool_sequence,
        'ast_tool_sequence': ast_tool_sequence,

        'ast_tool_recovery_distances': ast_tool_recovery_distances,
        'average_ast_tool_recovery_distance': avg_recovery,
        'max_ast_tool_recovery_distance': max_recovery,
    }

    if ast_tool_help_by_command:
        result['ast_tool_help_by_command'] = dict(ast_tool_help_by_command)

    result['consecutive_ast_tool_retries'] = consecutive_ast_tool_retries

    # Token metrics from result_record only — never estimated
    if result_record:
        tokens = result_record.get('tokens') or {}
        if tokens:
            inp = tokens.get('input')
            out = tokens.get('output')
            if inp is not None and out is not None:
                cache_read = tokens.get('cache_read', 0) or 0
                cache_creation = tokens.get('cache_creation', 0) or 0
                result['input_tokens'] = inp
                result['output_tokens'] = out
                if cache_read:
                    result['cache_read_tokens'] = cache_read
                if cache_creation:
                    result['cache_creation_tokens'] = cache_creation
                result['total_tokens'] = inp + out + cache_read + cache_creation

    return result


# ---------------------------------------------------------------------------
# Aggregation
# ---------------------------------------------------------------------------

def aggregate(summaries: list[dict[str, Any]]) -> dict[str, Any]:
    """Aggregate a list of per-task summaries into a single baseline report."""
    if not summaries:
        return {}

    tests = len(summaries)
    successes = sum(1 for s in summaries if s.get('success'))
    failures = tests - successes
    success_rate = successes / tests if tests else 0.0

    total_tool_calls = sum(s.get('total_tool_calls', 0) for s in summaries)
    avg_tool_calls = total_tool_calls / tests if tests else 0.0

    ast_tool_calls = sum(s.get('ast_tool_calls', 0) for s in summaries)
    ast_tool_failures = sum(s.get('ast_tool_failures', 0) for s in summaries)
    ast_tool_failure_rate = ast_tool_failures / ast_tool_calls if ast_tool_calls else 0.0
    ast_tool_help_calls = sum(s.get('ast_tool_help_calls', 0) for s in summaries)
    ast_tool_retries = sum(s.get('ast_tool_retries', 0) for s in summaries)

    bash_calls = sum(s.get('bash_calls', 0) for s in summaries)
    read_calls = sum(s.get('read_calls', 0) for s in summaries)
    edit_calls = sum(s.get('edit_calls', 0) for s in summaries)
    grep_calls = sum(s.get('grep_calls', 0) for s in summaries)
    glob_calls = sum(s.get('glob_calls', 0) for s in summaries)

    elapsed_list = [s.get('elapsed_seconds', 0.0) for s in summaries]
    total_elapsed = sum(elapsed_list)
    avg_elapsed = total_elapsed / tests if tests else 0.0

    # Per-command aggregates
    agg_commands: dict[str, int] = defaultdict(int)
    agg_failures_by_command: dict[str, int] = defaultdict(int)
    for s in summaries:
        for cmd, cnt in (s.get('ast_tool_commands') or {}).items():
            agg_commands[cmd] += cnt
        for cmd, cnt in (s.get('ast_tool_failures_by_command') or {}).items():
            agg_failures_by_command[cmd] += cnt

    # All recovery distances
    all_distances: list[int] = []
    for s in summaries:
        all_distances.extend(s.get('ast_tool_recovery_distances') or [])

    agg: dict[str, Any] = {
        'tests': tests,
        'successes': successes,
        'failures': failures,
        'success_rate': round(success_rate, 4),

        'total_tool_calls': total_tool_calls,
        'average_tool_calls_per_test': round(avg_tool_calls, 2),

        'ast_tool_calls': ast_tool_calls,
        'ast_tool_failures': ast_tool_failures,
        'ast_tool_failure_rate': round(ast_tool_failure_rate, 4),

        'ast_tool_help_calls': ast_tool_help_calls,
        'ast_tool_retries': ast_tool_retries,

        'bash_calls': bash_calls,
        'read_calls': read_calls,
        'edit_calls': edit_calls,
        'grep_calls': grep_calls,
        'glob_calls': glob_calls,

        'total_elapsed_seconds': round(total_elapsed, 2),
        'average_elapsed_seconds': round(avg_elapsed, 2),

        'ast_tool_commands': dict(agg_commands),
        'ast_tool_failures_by_command': dict(agg_failures_by_command),

        'all_ast_tool_recovery_distances': all_distances,
        'average_ast_tool_recovery_distance': (
            round(statistics.mean(all_distances), 2) if all_distances else None
        ),
        'max_ast_tool_recovery_distance': (
            max(all_distances) if all_distances else None
        ),
    }

    # Token metrics — only when available
    input_tokens = [s['input_tokens'] for s in summaries if 'input_tokens' in s]
    output_tokens = [s['output_tokens'] for s in summaries if 'output_tokens' in s]
    total_tokens = [s['total_tokens'] for s in summaries if 'total_tokens' in s]

    if input_tokens:
        agg['total_input_tokens'] = sum(input_tokens)
        agg['total_output_tokens'] = sum(output_tokens)
        agg['total_tokens'] = sum(total_tokens)
        agg['average_tokens_per_test'] = round(
            statistics.mean(total_tokens), 1
        ) if total_tokens else None

    return agg


# ---------------------------------------------------------------------------
# Loading helpers
# ---------------------------------------------------------------------------

def load_results_index(results_path: Path) -> dict[str, dict[str, Any]]:
    """Load results.jsonl and index by task_id."""
    index: dict[str, dict[str, Any]] = {}
    if not results_path.exists():
        return index
    with results_path.open('r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                rec = json.loads(line)
                tid = rec.get('task_id')
                if tid:
                    index[tid] = rec
            except json.JSONDecodeError:
                pass
    return index


def analyze_directory(
    trace_dir: Path,
    results_path: Path | None = None,
    task_filter: list[str] | None = None,
) -> list[dict[str, Any]]:
    """Analyze all (or selected) traces in trace_dir."""
    result_index: dict[str, dict[str, Any]] = {}
    if results_path:
        result_index = load_results_index(results_path)

    trace_files = sorted(
        p for p in trace_dir.glob('*.jsonl') if p.name != 'index.jsonl'
    )

    summaries: list[dict[str, Any]] = []
    for trace_path in trace_files:
        task_id = trace_path.stem
        if task_filter and task_id not in task_filter:
            continue
        try:
            rec = result_index.get(task_id)
            summary = analyze_trace(trace_path, result_record=rec)
            summaries.append(summary)
        except Exception as exc:
            print(f'[trace_analyzer] WARNING: failed to analyze {trace_path.name}: {exc}')

    return summaries


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description='Phase 0 baseline trace analyzer for ast-tool evaluation traces.'
    )
    parser.add_argument(
        '--trace-dir',
        type=Path,
        default=Path('evaluation/results/traces'),
        help='Directory containing JSONL trace files (default: evaluation/results/traces)',
    )
    parser.add_argument(
        '--results',
        type=Path,
        default=None,
        help='Optional path to results.jsonl for token metrics',
    )
    parser.add_argument(
        '--output-dir',
        type=Path,
        default=Path('evaluation/results/baseline'),
        help='Output directory for JSON summaries (default: evaluation/results/baseline)',
    )
    parser.add_argument(
        '--aggregate-only',
        action='store_true',
        help='Print only the aggregate summary, not per-task files',
    )
    parser.add_argument(
        'task_ids',
        nargs='*',
        help='Optional list of task IDs to analyze (default: all)',
    )
    return parser


def main() -> int:
    parser = _build_parser()
    args = parser.parse_args()

    trace_dir: Path = args.trace_dir
    if not trace_dir.exists():
        print(f'ERROR: trace directory not found: {trace_dir}')
        return 1

    summaries = analyze_directory(
        trace_dir,
        results_path=args.results,
        task_filter=args.task_ids or None,
    )

    if not summaries:
        print('No traces found or analyzed.')
        return 1

    output_dir: Path = args.output_dir
    output_dir.mkdir(parents=True, exist_ok=True)

    if not args.aggregate_only:
        for summary in summaries:
            out_path = output_dir / f"{summary['task_id']}.json"
            out_path.write_text(
                json.dumps(summary, indent=2), encoding='utf-8'
            )

        print(f'Wrote {len(summaries)} per-task summaries to {output_dir}/')

    agg = aggregate(summaries)
    agg_path = output_dir / 'aggregate.json'
    agg_path.write_text(json.dumps(agg, indent=2), encoding='utf-8')

    print(f'Aggregate baseline written to {agg_path}')
    print()
    print(f'Tests analyzed:      {agg["tests"]}')
    print(f'Success rate:        {agg["success_rate"]:.1%}')
    print(f'Total tool calls:    {agg["total_tool_calls"]}')
    print(f'ast-tool calls:      {agg["ast_tool_calls"]}')
    print(f'ast-tool failures:   {agg["ast_tool_failures"]} ({agg["ast_tool_failure_rate"]:.1%})')
    print(f'ast-tool retries:    {agg["ast_tool_retries"]}')
    print(f'ast-tool help calls: {agg["ast_tool_help_calls"]}')

    if agg['ast_tool_commands']:
        print()
        print('Commands:')
        for cmd, cnt in sorted(
            agg['ast_tool_commands'].items(), key=lambda x: -x[1]
        ):
            print(f'  {cmd:<20} {cnt:>6}')

    return 0


if __name__ == '__main__':
    raise SystemExit(main())
