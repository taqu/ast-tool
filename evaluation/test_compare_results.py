from __future__ import annotations

import json
import pytest
from pathlib import Path

from compare_results import (
    NormalizedResult,
    TaskComparison,
    build_task_comparisons,
    delta,
    get_level,
    load_jsonl,
    matched,
    normalize_result,
    select_latest_results,
    unmatched,
    write_ast_tool_command_usage_csv,
    write_ast_tool_success_failure_csv,
    write_success_comparison_csv,
    _all_ast_commands,
    _success_rows,
    _total_tokens,
)


# ---------------------------------------------------------------------------
# get_level
# ---------------------------------------------------------------------------

def test_get_level_smoke():
    assert get_level("smoke-001") == "smoke"


def test_get_level_level1():
    assert get_level("level1-001") == "level1"


def test_get_level_level2():
    assert get_level("level2-004") == "level2"


def test_get_level_no_dash():
    assert get_level("nodash") == "unknown"


# ---------------------------------------------------------------------------
# select_latest_results
# ---------------------------------------------------------------------------

def test_latest_result_wins_failure_then_success():
    records = [
        {"task_id": "task-001", "success": False},
        {"task_id": "task-002", "success": True},
        {"task_id": "task-001", "success": True},
    ]
    latest = select_latest_results(records)
    assert latest["task-001"]["success"] is True
    assert latest["task-002"]["success"] is True


def test_latest_result_wins_success_then_failure():
    records = [
        {"task_id": "task-001", "success": True},
        {"task_id": "task-001", "success": False},
    ]
    latest = select_latest_results(records)
    assert latest["task-001"]["success"] is False


def test_latest_result_single():
    records = [{"task_id": "task-001", "success": True}]
    latest = select_latest_results(records)
    assert "task-001" in latest


# ---------------------------------------------------------------------------
# normalize_result
# ---------------------------------------------------------------------------

def test_normalize_result_missing_tokens():
    record = {"task_id": "level1-001", "success": True}
    r = normalize_result(record)
    assert r.tokens["input"] is None
    assert r.tokens["output"] is None
    assert r.tokens["cache_read"] is None
    assert r.tokens["cache_creation"] is None


def test_normalize_result_present_tokens():
    record = {
        "task_id": "level1-001",
        "success": True,
        "tokens": {"input": 100, "output": 50, "cache_read": 200, "cache_creation": 10},
    }
    r = normalize_result(record)
    assert r.tokens["input"] == 100
    assert r.tokens["output"] == 50
    assert r.tokens["cache_read"] == 200
    assert r.tokens["cache_creation"] == 10


def test_normalize_result_partial_tokens():
    record = {
        "task_id": "level1-001",
        "success": True,
        "tokens": {"input": 100},
    }
    r = normalize_result(record)
    assert r.tokens["input"] == 100
    assert r.tokens["output"] is None


def test_normalize_result_elapsed_missing():
    record = {"task_id": "level1-001", "success": True}
    r = normalize_result(record)
    assert r.elapsed_seconds is None


def test_normalize_result_elapsed_present():
    record = {"task_id": "level1-001", "success": True, "elapsed_seconds": 42.5}
    r = normalize_result(record)
    assert r.elapsed_seconds == pytest.approx(42.5)


def test_normalize_result_tools_and_ast_tool():
    record = {
        "task_id": "level1-001",
        "success": True,
        "tools": {"Read": 3, "Bash": 2},
        "ast_tool": {"callers": 5, "search": 2},
    }
    r = normalize_result(record)
    assert r.tools["Read"] == 3
    assert r.tools["Bash"] == 2
    assert r.ast_tool["callers"] == 5
    assert r.ast_tool["search"] == 2


def test_normalize_result_level_from_task_id():
    record = {"task_id": "level3-007", "success": False}
    r = normalize_result(record)
    assert r.level == "level3"


# ---------------------------------------------------------------------------
# _total_tokens
# ---------------------------------------------------------------------------

def test_total_tokens_all_present():
    tokens = {"input": 100, "output": 50, "cache_read": 200, "cache_creation": 10}
    assert _total_tokens(tokens) == 360


def test_total_tokens_all_none():
    tokens = {"input": None, "output": None, "cache_read": None, "cache_creation": None}
    assert _total_tokens(tokens) is None


def test_total_tokens_partial():
    tokens = {"input": 100, "output": None, "cache_read": 50, "cache_creation": None}
    assert _total_tokens(tokens) == 150


# ---------------------------------------------------------------------------
# build_task_comparisons / matched / unmatched
# ---------------------------------------------------------------------------

def _make_result(task_id: str, success: bool = True) -> NormalizedResult:
    return NormalizedResult(
        task_id=task_id,
        level=get_level(task_id),
        success=success,
        elapsed_seconds=10.0,
        tokens={"input": 100, "output": 50, "cache_read": 0, "cache_creation": 0},
        tools={"Read": 2},
        ast_tool={},
        workflow=[],
        changed_files=[],
        validation=None,
    )


def test_task_matching_basic():
    with_ast = {
        "task-001": _make_result("task-001"),
        "task-002": _make_result("task-002"),
    }
    without_ast = {
        "task-001": _make_result("task-001"),
        "task-003": _make_result("task-003"),
    }

    comparisons = build_task_comparisons(with_ast, without_ast)
    mc = matched(comparisons)
    um = unmatched(comparisons)

    matched_ids = {c.task_id for c in mc}
    unmatched_ids = {c.task_id for c in um}

    assert matched_ids == {"task-001"}
    assert unmatched_ids == {"task-002", "task-003"}


def test_task_matching_all_matched():
    with_ast = {
        "task-001": _make_result("task-001"),
        "task-002": _make_result("task-002"),
    }
    without_ast = {
        "task-001": _make_result("task-001"),
        "task-002": _make_result("task-002"),
    }

    comparisons = build_task_comparisons(with_ast, without_ast)
    assert len(matched(comparisons)) == 2
    assert len(unmatched(comparisons)) == 0


def test_task_matching_none_matched():
    with_ast = {"task-001": _make_result("task-001")}
    without_ast = {"task-002": _make_result("task-002")}

    comparisons = build_task_comparisons(with_ast, without_ast)
    assert len(matched(comparisons)) == 0
    assert len(unmatched(comparisons)) == 2


# ---------------------------------------------------------------------------
# success comparison
# ---------------------------------------------------------------------------

def _cmp(task_id: str, w_success: bool, wo_success: bool) -> TaskComparison:
    w = _make_result(task_id, w_success)
    wo = _make_result(task_id, wo_success)
    return TaskComparison(
        task_id=task_id,
        level=get_level(task_id),
        with_ast=w,
        without_ast=wo,
    )


def test_success_comparison_both_success():
    rows = _success_rows([_cmp("level1-001", True, True)])
    overall = rows[0]
    assert overall["both_success"] == 1
    assert overall["both_failure"] == 0
    assert overall["improved_with_ast"] == 0
    assert overall["regressed_with_ast"] == 0


def test_success_comparison_both_failure():
    rows = _success_rows([_cmp("level1-001", False, False)])
    overall = rows[0]
    assert overall["both_failure"] == 1
    assert overall["both_success"] == 0
    assert overall["improved_with_ast"] == 0
    assert overall["regressed_with_ast"] == 0


def test_success_comparison_improved():
    rows = _success_rows([_cmp("level1-001", True, False)])
    overall = rows[0]
    assert overall["improved_with_ast"] == 1
    assert overall["regressed_with_ast"] == 0


def test_success_comparison_regressed():
    rows = _success_rows([_cmp("level1-001", False, True)])
    overall = rows[0]
    assert overall["regressed_with_ast"] == 1
    assert overall["improved_with_ast"] == 0


# ---------------------------------------------------------------------------
# delta calculation
# ---------------------------------------------------------------------------

def test_delta_positive():
    assert delta(120.0, 100.0) == pytest.approx(20.0)


def test_delta_negative():
    assert delta(80.0, 100.0) == pytest.approx(-20.0)


def test_delta_none_left():
    assert delta(None, 100.0) is None


def test_delta_none_right():
    assert delta(120.0, None) is None


def test_delta_both_none():
    assert delta(None, None) is None


# ---------------------------------------------------------------------------
# missing token data must not become zero
# ---------------------------------------------------------------------------

def test_missing_tokens_not_zero():
    record = {"task_id": "level1-001", "success": True}
    r = normalize_result(record)
    assert r.tokens["input"] is None
    assert r.tokens["output"] is None
    assert r.tokens["cache_read"] is None
    assert r.tokens["cache_creation"] is None
    assert _total_tokens(r.tokens) is None


# ---------------------------------------------------------------------------
# tool union
# ---------------------------------------------------------------------------

def test_tool_union_includes_only_ast_side():
    w = _make_result("level1-001")
    w.tools = {"Read": 3, "Bash": 1}

    wo = _make_result("level1-001")
    wo.tools = {"Grep": 5}

    c = TaskComparison(
        task_id="level1-001",
        level="level1",
        with_ast=w,
        without_ast=wo,
    )

    from compare_results import _all_tools
    tools = _all_tools([c])
    assert "Read" in tools
    assert "Bash" in tools
    assert "Grep" in tools


# ---------------------------------------------------------------------------
# ast-tool command aggregation
# ---------------------------------------------------------------------------

def _make_result_with_ast_tool(
    task_id: str,
    success: bool,
    ast_tool: dict[str, int],
) -> NormalizedResult:
    r = _make_result(task_id, success)
    r.ast_tool = ast_tool
    return r


def test_ast_tool_all_commands_collected():
    with_ast = {
        "level1-001": _make_result_with_ast_tool(
            "level1-001", True, {"callers": 3, "search": 1}
        ),
        "level1-002": _make_result_with_ast_tool(
            "level1-002", False, {"references": 2}
        ),
    }

    commands = _all_ast_commands(with_ast)
    assert "callers" in commands
    assert "search" in commands
    assert "references" in commands


def test_ast_tool_success_failure_counts(tmp_path: Path):
    with_ast = {
        "level1-001": _make_result_with_ast_tool(
            "level1-001", True, {"callers": 3}
        ),
        "level1-002": _make_result_with_ast_tool(
            "level1-002", True, {"callers": 2}
        ),
        "level1-003": _make_result_with_ast_tool(
            "level1-003", False, {"callers": 5}
        ),
    }

    out = tmp_path / "ast_tool_success_failure.csv"
    write_ast_tool_success_failure_csv(with_ast, out)

    import csv
    rows = list(csv.DictReader(out.open("r", encoding="utf-8")))
    callers_row = next(r for r in rows if r["command"] == "callers")

    assert int(callers_row["successful_tasks_using"]) == 2
    assert int(callers_row["successful_total_calls"]) == 5
    assert int(callers_row["failed_tasks_using"]) == 1
    assert int(callers_row["failed_total_calls"]) == 5


def test_ast_tool_aggregation_by_level(tmp_path: Path):
    with_ast = {
        "level1-001": _make_result_with_ast_tool(
            "level1-001", True, {"search": 2}
        ),
        "level2-001": _make_result_with_ast_tool(
            "level2-001", True, {"search": 4}
        ),
    }

    out = tmp_path / "ast_tool_command_usage.csv"
    write_ast_tool_command_usage_csv(with_ast, out)

    import csv
    rows = list(csv.DictReader(out.open("r", encoding="utf-8")))
    search_row = next(r for r in rows if r["command"] == "search")

    assert int(search_row["total"]) == 6
    assert int(search_row["level1"]) == 2
    assert int(search_row["level2"]) == 4


# ---------------------------------------------------------------------------
# load_jsonl
# ---------------------------------------------------------------------------

def test_load_jsonl_valid(tmp_path: Path):
    content = (
        '{"task_id": "level1-001", "success": true}\n'
        '{"task_id": "level1-002", "success": false}\n'
    )
    p = tmp_path / "results.jsonl"
    p.write_text(content, encoding="utf-8")
    records = load_jsonl(p)
    assert len(records) == 2


def test_load_jsonl_skips_malformed(tmp_path: Path, capsys):
    content = (
        '{"task_id": "level1-001", "success": true}\n'
        'not valid json\n'
        '{"task_id": "level1-002", "success": false}\n'
    )
    p = tmp_path / "results.jsonl"
    p.write_text(content, encoding="utf-8")
    records = load_jsonl(p)
    assert len(records) == 2
    captured = capsys.readouterr()
    assert "WARNING" in captured.out


def test_load_jsonl_empty_lines(tmp_path: Path):
    content = (
        '{"task_id": "level1-001", "success": true}\n'
        '\n'
        '{"task_id": "level1-002", "success": false}\n'
    )
    p = tmp_path / "results.jsonl"
    p.write_text(content, encoding="utf-8")
    records = load_jsonl(p)
    assert len(records) == 2
