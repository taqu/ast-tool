#!/usr/bin/env python3
"""Unit tests for trace_analyzer.py (Phase 0 baseline metrics)."""
from __future__ import annotations

import json
from pathlib import Path

import pytest

from trace_analyzer import analyze_trace, aggregate


# ---------------------------------------------------------------------------
# Trace-building helpers
# ---------------------------------------------------------------------------

def _make_trace(
    task_id: str,
    tool_calls: list[dict],
    success: bool = True,
    validation_success: bool = True,
    elapsed: float = 10.0,
) -> dict:
    """Bundle trace components; written by _write_trace()."""
    return {
        'task_id': task_id,
        'tool_calls': tool_calls,
        'success': success,
        'validation_success': validation_success,
        'elapsed': elapsed,
    }


def _write_trace(tmp_path: Path, trace: dict) -> Path:
    task_id = trace['task_id']
    path = tmp_path / f'{task_id}.jsonl'
    with path.open('w', encoding='utf-8') as f:
        f.write(json.dumps({
            'event': 'task_start',
            'task_id': task_id,
            'agent': 'claude',
            'repository': 'repos/test',
        }) + '\n')
        for seq, tc in enumerate(trace['tool_calls'], start=1):
            ev: dict = {
                'event': 'tool_call',
                'sequence': seq,
                'tool': tc['tool'],
                'tool_use_id': f'id{seq}',
                'input': tc.get('input', {}),
                'output': tc.get('output', ''),
                'success': tc.get('success', True),
            }
            if 'ast_tool' in tc:
                ev['ast_tool'] = tc['ast_tool']
            f.write(json.dumps(ev) + '\n')
        f.write(json.dumps({
            'event': 'task_end',
            'task_id': task_id,
            'tool_calls': len(trace['tool_calls']),
            'success': trace['success'],
            'validation_success': trace['validation_success'],
            'elapsed_seconds': trace['elapsed'],
        }) + '\n')
    return path


def _ast_call(command: str, success: bool = True, flags: str = '') -> dict:
    """Create a Bash tool_call event invoking ast-tool with pre-annotation."""
    raw = f'ast-tool {command} some_arg {flags}'.strip()
    return {
        'tool': 'Bash',
        'input': {'command': raw},
        'output': 'ok' if success else 'error: not found',
        'success': success,
        'ast_tool': {
            'detected': True,
            'command': command,
            'raw_command': raw,
        },
    }


def _tool_call(tool: str, success: bool = True) -> dict:
    return {
        'tool': tool,
        'input': {},
        'output': 'ok',
        'success': success,
    }


# ---------------------------------------------------------------------------
# Test 1: Successful AST Tool command
# ---------------------------------------------------------------------------

def test_successful_ast_tool_command(tmp_path: Path):
    path = _write_trace(tmp_path, _make_trace(
        'level2-001',
        [_ast_call('search', success=True)],
    ))
    s = analyze_trace(path)
    assert s['ast_tool_calls'] == 1
    assert s['ast_tool_successes'] == 1
    assert s['ast_tool_failures'] == 0


# ---------------------------------------------------------------------------
# Test 2: Failed AST Tool command
# ---------------------------------------------------------------------------

def test_failed_ast_tool_command(tmp_path: Path):
    path = _write_trace(tmp_path, _make_trace(
        'level2-002',
        [_ast_call('callers', success=False)],
    ))
    s = analyze_trace(path)
    assert s['ast_tool_calls'] == 1
    assert s['ast_tool_failures'] == 1
    assert s['ast_tool_successes'] == 0
    assert s['ast_tool_failures_by_command'].get('callers') == 1


# ---------------------------------------------------------------------------
# Test 3: Retry detection
# ---------------------------------------------------------------------------

def test_retry_detection(tmp_path: Path):
    # callers → failure, search → success, callers → success
    path = _write_trace(tmp_path, _make_trace(
        'level2-003',
        [
            _ast_call('callers', success=False),
            _ast_call('search', success=True),
            _ast_call('callers', success=True),
        ],
    ))
    s = analyze_trace(path)
    assert s['ast_tool_retries'] == 1


def test_retry_multiple(tmp_path: Path):
    # callers → fail, callers → fail, callers → success: 2 retries
    path = _write_trace(tmp_path, _make_trace(
        'level2-009',
        [
            _ast_call('callers', success=False),
            _ast_call('callers', success=False),
            _ast_call('callers', success=True),
        ],
    ))
    s = analyze_trace(path)
    assert s['ast_tool_retries'] == 2


def test_no_retry_without_prior_failure(tmp_path: Path):
    path = _write_trace(tmp_path, _make_trace(
        'level1-001',
        [
            _ast_call('search', success=True),
            _ast_call('search', success=True),
        ],
    ))
    s = analyze_trace(path)
    assert s['ast_tool_retries'] == 0


# ---------------------------------------------------------------------------
# Test 4: Help detection
# ---------------------------------------------------------------------------

def test_help_detection_with_command(tmp_path: Path):
    ev = {
        'tool': 'Bash',
        'input': {'command': 'ast-tool callers --help'},
        'output': 'usage: ...',
        'success': True,
        'ast_tool': {
            'detected': True,
            'command': 'callers',
            'raw_command': 'ast-tool callers --help',
        },
    }
    path = _write_trace(tmp_path, _make_trace('level1-002', [ev]))
    s = analyze_trace(path)
    assert s['ast_tool_help_calls'] == 1
    assert s['ast_tool_calls'] == 1


def test_help_detection_toplevel(tmp_path: Path):
    # ast-tool --help is NOT caught by tracer's \w+ regex, so no ast_tool annotation
    ev = {
        'tool': 'Bash',
        'input': {'command': 'ast-tool --help'},
        'output': 'usage: ast-tool ...',
        'success': True,
        # No ast_tool annotation — must be detected by fallback
    }
    path = _write_trace(tmp_path, _make_trace('level1-003', [ev]))
    s = analyze_trace(path)
    assert s['ast_tool_help_calls'] == 1
    assert s['ast_tool_calls'] == 1


# ---------------------------------------------------------------------------
# Test 5: Pretty JSON detection
# ---------------------------------------------------------------------------

def test_pretty_json_detection(tmp_path: Path):
    ev = _ast_call('search', success=True, flags='--json --pretty')
    # Patch raw_command to include flags
    ev['ast_tool']['raw_command'] = 'ast-tool search --json --pretty .'
    ev['input']['command'] = ev['ast_tool']['raw_command']
    path = _write_trace(tmp_path, _make_trace('level2-004', [ev]))
    s = analyze_trace(path)
    assert s['ast_tool_json_calls'] == 1
    assert s['ast_tool_pretty_json_calls'] == 1


# ---------------------------------------------------------------------------
# Test 6: Non-pretty JSON
# ---------------------------------------------------------------------------

def test_json_no_pretty(tmp_path: Path):
    ev = _ast_call('search', success=True, flags='--json')
    ev['ast_tool']['raw_command'] = 'ast-tool search --json .'
    ev['input']['command'] = ev['ast_tool']['raw_command']
    path = _write_trace(tmp_path, _make_trace('level2-005', [ev]))
    s = analyze_trace(path)
    assert s['ast_tool_json_calls'] == 1
    assert s['ast_tool_pretty_json_calls'] == 0


# ---------------------------------------------------------------------------
# Test 7: No AST Tool usage
# ---------------------------------------------------------------------------

def test_no_ast_tool_usage(tmp_path: Path):
    path = _write_trace(tmp_path, _make_trace(
        'level1-004',
        [
            _tool_call('Read'),
            _tool_call('Grep'),
            _tool_call('Edit'),
        ],
    ))
    s = analyze_trace(path)
    assert s['ast_tool_calls'] == 0
    assert s['ast_tool_successes'] == 0
    assert s['ast_tool_failures'] == 0
    assert s['read_calls'] == 1
    assert s['grep_calls'] == 1
    assert s['edit_calls'] == 1


# ---------------------------------------------------------------------------
# Test 8: Unknown AST Tool command
# ---------------------------------------------------------------------------

def test_unknown_ast_tool_command(tmp_path: Path):
    ev = {
        'tool': 'Bash',
        'input': {'command': 'ast-tool future-command --some-flag'},
        'output': 'result',
        'success': True,
        'ast_tool': {
            'detected': True,
            'command': 'future-command',
            'raw_command': 'ast-tool future-command --some-flag',
        },
    }
    path = _write_trace(tmp_path, _make_trace('level3-001', [ev]))
    s = analyze_trace(path)
    assert s['ast_tool_commands'].get('future-command') == 1
    assert s['ast_tool_calls'] == 1


# ---------------------------------------------------------------------------
# Test 9: Recovery distance
# ---------------------------------------------------------------------------

def test_recovery_distance_simple(tmp_path: Path):
    # callers failure → search success: distance 1
    path = _write_trace(tmp_path, _make_trace(
        'level2-006',
        [
            _ast_call('callers', success=False),
            _ast_call('search', success=True),
        ],
    ))
    s = analyze_trace(path)
    assert s['ast_tool_recovery_distances'] == [1]
    assert s['average_ast_tool_recovery_distance'] == 1.0
    assert s['max_ast_tool_recovery_distance'] == 1


def test_recovery_distance_with_intermediary(tmp_path: Path):
    # callers failure, find failure, [non-ast], symbols success
    path = _write_trace(tmp_path, _make_trace(
        'level2-007',
        [
            _ast_call('callers', success=False),
            _ast_call('find', success=False),
            _tool_call('Bash'),              # non-ast
            _ast_call('symbols', success=True),
        ],
    ))
    s = analyze_trace(path)
    # From callers-fail: find(1), bash(2), symbols(3) → distance 3
    # From find-fail: bash(1), symbols(2) → distance 2
    assert 3 in s['ast_tool_recovery_distances']
    assert 2 in s['ast_tool_recovery_distances']


def test_recovery_distance_none_when_no_failure(tmp_path: Path):
    path = _write_trace(tmp_path, _make_trace(
        'level1-005',
        [_ast_call('search', success=True)],
    ))
    s = analyze_trace(path)
    assert s['ast_tool_recovery_distances'] == []
    assert s['average_ast_tool_recovery_distance'] is None
    assert s['max_ast_tool_recovery_distance'] is None


def test_recovery_distance_no_recovery(tmp_path: Path):
    # failure with no subsequent success → not recorded
    path = _write_trace(tmp_path, _make_trace(
        'level4-001',
        [
            _ast_call('callers', success=False),
            _tool_call('Read'),
        ],
        success=False,
    ))
    s = analyze_trace(path)
    assert s['ast_tool_recovery_distances'] == []


# ---------------------------------------------------------------------------
# Test: Tool sequence
# ---------------------------------------------------------------------------

def test_tool_sequence(tmp_path: Path):
    path = _write_trace(tmp_path, _make_trace(
        'level2-008',
        [
            _tool_call('Skill'),
            _ast_call('callers', success=False),
            _ast_call('search', success=True),
            _tool_call('Read'),
            _tool_call('Edit'),
        ],
    ))
    s = analyze_trace(path)
    assert s['tool_sequence'] == [
        'Skill',
        'ast-tool:callers',
        'ast-tool:search',
        'Read',
        'Edit',
    ]
    assert s['ast_tool_sequence'] == ['callers', 'search']


# ---------------------------------------------------------------------------
# Test: Task level parsing
# ---------------------------------------------------------------------------

def test_task_level_parsed(tmp_path: Path):
    path = _write_trace(tmp_path, _make_trace('level3-007', []))
    s = analyze_trace(path)
    assert s['task_level'] == 3


def test_task_level_unknown(tmp_path: Path):
    path = _write_trace(tmp_path, _make_trace('smoke-001', []))
    s = analyze_trace(path)
    assert s['task_level'] is None


# ---------------------------------------------------------------------------
# Test: Tool counts
# ---------------------------------------------------------------------------

def test_tool_counts(tmp_path: Path):
    path = _write_trace(tmp_path, _make_trace(
        'level1-006',
        [
            _tool_call('Bash'),
            _tool_call('Bash'),
            _tool_call('Read'),
            _tool_call('Grep'),
            _tool_call('Edit'),
            _tool_call('Skill'),
        ],
    ))
    s = analyze_trace(path)
    assert s['bash_calls'] == 2
    assert s['read_calls'] == 1
    assert s['grep_calls'] == 1
    assert s['edit_calls'] == 1
    assert s['skill_calls'] == 1
    assert s['total_tool_calls'] == 6


# ---------------------------------------------------------------------------
# Test: Token metrics from result_record
# ---------------------------------------------------------------------------

def test_token_metrics_from_result_record(tmp_path: Path):
    path = _write_trace(tmp_path, _make_trace('level2-010', []))
    rec = {
        'task_id': 'level2-010',
        'tokens': {'input': 1000, 'output': 500, 'cache_read': 200, 'cache_creation': 0},
    }
    s = analyze_trace(path, result_record=rec)
    assert s['input_tokens'] == 1000
    assert s['output_tokens'] == 500
    assert s['cache_read_tokens'] == 200
    assert s['total_tokens'] == 1700


def test_token_metrics_absent_when_no_record(tmp_path: Path):
    path = _write_trace(tmp_path, _make_trace('level2-011', []))
    s = analyze_trace(path, result_record=None)
    assert 'input_tokens' not in s
    assert 'total_tokens' not in s


# ---------------------------------------------------------------------------
# Test: Malformed events are skipped
# ---------------------------------------------------------------------------

def test_malformed_line_skipped(tmp_path: Path):
    task_id = 'level1-007'
    path = tmp_path / f'{task_id}.jsonl'
    path.write_text(
        json.dumps({'event': 'task_start', 'task_id': task_id}) + '\n'
        + 'NOT JSON AT ALL\n'
        + json.dumps({
            'event': 'tool_call', 'sequence': 1, 'tool': 'Read',
            'tool_use_id': 'id1', 'input': {}, 'output': '', 'success': True,
        }) + '\n'
        + json.dumps({'event': 'task_end', 'task_id': task_id, 'success': True, 'elapsed_seconds': 5.0}) + '\n',
        encoding='utf-8',
    )
    s = analyze_trace(path)
    assert s['read_calls'] == 1
    assert s['total_tool_calls'] == 1


# ---------------------------------------------------------------------------
# Test: Aggregate
# ---------------------------------------------------------------------------

def test_aggregate_basic(tmp_path: Path):
    p1 = _write_trace(tmp_path, _make_trace(
        'level1-001',
        [_ast_call('search', success=True), _tool_call('Edit')],
        success=True, elapsed=10.0,
    ))
    p2 = _write_trace(tmp_path, _make_trace(
        'level2-001',
        [_ast_call('callers', success=False), _ast_call('callers', success=True)],
        success=True, elapsed=20.0,
    ))
    s1 = analyze_trace(p1)
    s2 = analyze_trace(p2)
    agg = aggregate([s1, s2])

    assert agg['tests'] == 2
    assert agg['successes'] == 2
    assert agg['success_rate'] == 1.0
    assert agg['ast_tool_calls'] == 3  # 1 + 2
    assert agg['ast_tool_failures'] == 1
    assert agg['ast_tool_retries'] == 1
    assert agg['total_elapsed_seconds'] == pytest.approx(30.0)
    assert agg['ast_tool_commands']['search'] == 1
    assert agg['ast_tool_commands']['callers'] == 2


def test_aggregate_empty():
    result = aggregate([])
    assert result == {}


def test_aggregate_token_metrics_partial(tmp_path: Path):
    p1 = _write_trace(tmp_path, _make_trace('level1-001', []))
    p2 = _write_trace(tmp_path, _make_trace('level1-002', []))
    rec = {'task_id': 'level1-001', 'tokens': {'input': 100, 'output': 50}}
    s1 = analyze_trace(p1, result_record=rec)
    s2 = analyze_trace(p2, result_record=None)
    agg = aggregate([s1, s2])
    # Only s1 had token data
    assert agg['total_tokens'] == 150


# ---------------------------------------------------------------------------
# Integration smoke test: existing traces
# ---------------------------------------------------------------------------

TRACE_DIR = Path('evaluation/results/traces')


@pytest.mark.skipif(
    not TRACE_DIR.exists(),
    reason='evaluation traces not present',
)
def test_integration_level2_traces():
    """Analyze all level2 traces and verify basic structural invariants."""
    traces = sorted(TRACE_DIR.glob('level2-*.jsonl'))
    assert traces, 'No level2 traces found'

    for trace_path in traces:
        s = analyze_trace(trace_path)
        assert s['task_level'] == 2
        assert isinstance(s['total_tool_calls'], int)
        assert isinstance(s['ast_tool_calls'], int)
        assert s['ast_tool_calls'] == s['ast_tool_successes'] + s['ast_tool_failures']
        assert isinstance(s['tool_sequence'], list)
        assert isinstance(s['ast_tool_sequence'], list)
        assert len(s['ast_tool_sequence']) == s['ast_tool_calls']
        assert s['ast_tool_retries'] >= 0
        assert s['ast_tool_help_calls'] >= 0


@pytest.mark.skipif(
    not TRACE_DIR.exists(),
    reason='evaluation traces not present',
)
def test_integration_all_traces():
    """Analyze every trace; verify aggregate sums are consistent."""
    traces = [p for p in TRACE_DIR.glob('*.jsonl') if p.name != 'index.jsonl']
    assert traces

    summaries = [analyze_trace(p) for p in traces]
    agg = aggregate(summaries)

    total_ast = sum(s['ast_tool_calls'] for s in summaries)
    assert agg['ast_tool_calls'] == total_ast

    total_tools = sum(s['total_tool_calls'] for s in summaries)
    assert agg['total_tool_calls'] == total_tools
