import json
import pytest
from pathlib import Path
from runner import load_latest_results, should_run_task

def test_load_latest_results_missing(tmp_path):
    results_path = tmp_path / "results.jsonl"
    latest = load_latest_results(results_path)
    assert latest == {}

def test_load_latest_results_malformed(tmp_path, capsys):
    results_path = tmp_path / "results.jsonl"
    content = (
        '{"agent": "claude", "task_id": "task-001", "success": true}\n'
        'malformed json line here\n'
        '{"agent": "claude", "task_id": "task-002", "success": false}\n'
    )
    results_path.write_text(content, encoding="utf-8")
    latest = load_latest_results(results_path)
    
    # Check that we parsed the valid lines
    assert ("claude", "task-001") in latest
    assert latest[("claude", "task-001")]["success"] is True
    assert ("claude", "task-002") in latest
    assert latest[("claude", "task-002")]["success"] is False
    
    # Check warning
    captured = capsys.readouterr()
    assert f"Warning: invalid JSON in {results_path}:2" in captured.out or f"Warning: invalid JSON in {results_path}:2" in captured.err

def test_load_latest_results_backward_compatibility(tmp_path):
    results_path = tmp_path / "results.jsonl"
    # No agent field
    content = '{"task_id": "task-001", "success": true}\n'
    results_path.write_text(content, encoding="utf-8")
    latest = load_latest_results(results_path)
    # Should default to "claude"
    assert ("claude", "task-001") in latest
    assert latest[("claude", "task-001")]["success"] is True

def test_should_run_task_modes():
    # Setup latest_results mapping
    latest_results = {
        ("claude", "task-passed"): {"success": True},
        ("claude", "task-failed"): {"success": False},
        ("antigravity", "task-passed"): {"success": True},
        ("antigravity", "task-failed"): {"success": False},
    }
    
    # Mode: resume (default)
    # - success -> skip
    assert should_run_task("task-passed", "claude", latest_results, "resume") is False
    # - failure -> run
    assert should_run_task("task-failed", "claude", latest_results, "resume") is True
    # - missing -> run
    assert should_run_task("task-missing", "claude", latest_results, "resume") is True

    # Mode: retry-failed
    # - success -> skip
    assert should_run_task("task-passed", "claude", latest_results, "retry-failed") is False
    # - failure -> run
    assert should_run_task("task-failed", "claude", latest_results, "retry-failed") is True
    # - missing -> skip
    assert should_run_task("task-missing", "claude", latest_results, "retry-failed") is False

    # Mode: force
    # - success -> run
    assert should_run_task("task-passed", "claude", latest_results, "force") is True
    # - failure -> run
    assert should_run_task("task-failed", "claude", latest_results, "force") is True
    # - missing -> run
    assert should_run_task("task-missing", "claude", latest_results, "force") is True

def test_agent_separation():
    latest_results = {
        ("claude", "task-001"): {"success": True},
        ("antigravity", "task-001"): {"success": False},
    }
    # For claude, task-001 should skip
    assert should_run_task("task-001", "claude", latest_results, "resume") is False
    # For antigravity, task-001 should run
    assert should_run_task("task-001", "antigravity", latest_results, "resume") is True

def test_latest_result_wins(tmp_path):
    results_path = tmp_path / "results.jsonl"
    content = (
        '{"agent": "claude", "task_id": "task-001", "success": false}\n'
        '{"agent": "claude", "task_id": "task-001", "success": true}\n'
    )
    results_path.write_text(content, encoding="utf-8")
    latest = load_latest_results(results_path)
    assert latest[("claude", "task-001")]["success"] is True
