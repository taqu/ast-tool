from __future__ import annotations

import json
import pytest
from pathlib import Path

from tracer import (
    detect_ast_tool,
    parse_trace_events,
    write_task_trace,
    append_trace_index,
    _maybe_truncate,
    _extract_output_text,
)


# ---------------------------------------------------------------------------
# detect_ast_tool
# ---------------------------------------------------------------------------

def test_detect_ast_tool_callers():
    result = detect_ast_tool("Bash", {"command": "ast-tool callers greet"})
    assert result is not None
    assert result["detected"] is True
    assert result["command"] == "callers"
    assert "ast-tool callers greet" in result["raw_command"]


def test_detect_ast_tool_search():
    result = detect_ast_tool("Bash", {"command": "ast-tool search greet src/"})
    assert result is not None
    assert result["command"] == "search"


def test_detect_ast_tool_preserves_raw_command():
    cmd = "ast-tool callers some_func --extra-flag"
    result = detect_ast_tool("Bash", {"command": cmd})
    assert result is not None
    assert result["raw_command"] == cmd


def test_detect_ast_tool_not_bash():
    result = detect_ast_tool("Read", {"file_path": "src/main.cpp"})
    assert result is None


def test_detect_ast_tool_bash_no_ast_tool():
    result = detect_ast_tool("Bash", {"command": "ls -la src/"})
    assert result is None


def test_detect_ast_tool_string_input():
    result = detect_ast_tool("Bash", "ast-tool references foo")
    assert result is not None
    assert result["command"] == "references"


def test_detect_ast_tool_various_commands():
    commands = ["search", "find", "references", "callers", "callees", "symbols", "outline"]
    for cmd in commands:
        result = detect_ast_tool("Bash", {"command": f"ast-tool {cmd} foo"})
        assert result is not None, f"Expected detection for {cmd}"
        assert result["command"] == cmd


# ---------------------------------------------------------------------------
# _extract_output_text
# ---------------------------------------------------------------------------

def test_extract_output_string():
    assert _extract_output_text("hello") == "hello"


def test_extract_output_none():
    assert _extract_output_text(None) == ""


def test_extract_output_list_of_dicts():
    content = [{"text": "line 1"}, {"text": "line 2"}]
    result = _extract_output_text(content)
    assert "line 1" in result
    assert "line 2" in result


def test_extract_output_list_of_strings():
    content = ["foo", "bar"]
    result = _extract_output_text(content)
    assert "foo" in result
    assert "bar" in result


# ---------------------------------------------------------------------------
# _maybe_truncate
# ---------------------------------------------------------------------------

def test_maybe_truncate_no_limit():
    text = "hello world"
    out, truncated, orig = _maybe_truncate(text, None)
    assert out == text
    assert not truncated


def test_maybe_truncate_within_limit():
    text = "hello"
    out, truncated, orig = _maybe_truncate(text, 100)
    assert out == text
    assert not truncated


def test_maybe_truncate_exceeds_limit():
    text = "a" * 200
    out, truncated, orig = _maybe_truncate(text, 100)
    assert len(out.encode("utf-8")) <= 100
    assert truncated
    assert orig == 200


# ---------------------------------------------------------------------------
# parse_trace_events — driven by synthetic log files
# ---------------------------------------------------------------------------

def _make_log_dir(tmp_path: Path, entries: list[dict]) -> Path:
    """Write a synthetic Claude session log to tmp_path."""
    log_dir = tmp_path / "projects" / "test-project"
    log_dir.mkdir(parents=True)
    log_file = log_dir / "session.jsonl"

    with log_file.open("w", encoding="utf-8") as f:
        for entry in entries:
            f.write(json.dumps(entry) + "\n")

    return tmp_path / "projects"


def _assistant_tool_use(uid: str, name: str, input_: dict, ts: str = "2026-08-26T12:00:00.000Z") -> dict:
    return {
        "type": "assistant",
        "timestamp": ts,
        "message": {
            "role": "assistant",
            "content": [
                {"type": "tool_use", "id": uid, "name": name, "input": input_}
            ],
        },
    }


def _user_tool_result(uid: str, content: str, is_error: bool = False, ts: str = "2026-08-26T12:00:01.000Z") -> dict:
    return {
        "type": "user",
        "timestamp": ts,
        "message": {
            "role": "user",
            "content": [
                {"type": "tool_result", "tool_use_id": uid, "content": content, "is_error": is_error}
            ],
        },
    }


def test_parse_trace_events_basic(tmp_path: Path):
    entries = [
        _assistant_tool_use("id1", "Read", {"file_path": "src/main.cpp"}),
        _user_tool_result("id1", "int main() { ... }"),
    ]
    log_dir = _make_log_dir(tmp_path, entries)
    events = parse_trace_events(log_dir)

    assert len(events) == 1
    e = events[0]
    assert e["event"] == "tool_call"
    assert e["tool"] == "Read"
    assert e["input"] == {"file_path": "src/main.cpp"}
    assert e["output"] == "int main() { ... }"
    assert e["success"] is True


def test_parse_trace_events_sequence_ordering(tmp_path: Path):
    entries = [
        _assistant_tool_use("id1", "Read", {"file_path": "a.cpp"}, "2026-08-26T12:00:00.000Z"),
        _user_tool_result("id1", "content a", ts="2026-08-26T12:00:01.000Z"),
        _assistant_tool_use("id2", "Bash", {"command": "ls"}, "2026-08-26T12:00:02.000Z"),
        _user_tool_result("id2", "file1\nfile2", ts="2026-08-26T12:00:03.000Z"),
        _assistant_tool_use("id3", "Edit", {"file_path": "b.cpp"}, "2026-08-26T12:00:04.000Z"),
        _user_tool_result("id3", "ok", ts="2026-08-26T12:00:05.000Z"),
    ]
    log_dir = _make_log_dir(tmp_path, entries)
    events = parse_trace_events(log_dir)

    assert len(events) == 3
    assert events[0]["sequence"] == 1
    assert events[1]["sequence"] == 2
    assert events[2]["sequence"] == 3
    assert events[0]["tool"] == "Read"
    assert events[1]["tool"] == "Bash"
    assert events[2]["tool"] == "Edit"


def test_parse_trace_events_failed_tool_call(tmp_path: Path):
    entries = [
        _assistant_tool_use("id1", "Bash", {"command": "ast-tool callers unknown_sym"}),
        _user_tool_result("id1", "symbol not found", is_error=True),
    ]
    log_dir = _make_log_dir(tmp_path, entries)
    events = parse_trace_events(log_dir)

    assert len(events) == 1
    e = events[0]
    assert e["success"] is False
    assert e["output"] == "symbol not found"


def test_parse_trace_events_ast_tool_detected(tmp_path: Path):
    entries = [
        _assistant_tool_use("id1", "Bash", {"command": "ast-tool callers greet"}),
        _user_tool_result("id1", "src/main.cpp:5"),
    ]
    log_dir = _make_log_dir(tmp_path, entries)
    events = parse_trace_events(log_dir)

    assert len(events) == 1
    e = events[0]
    assert "ast_tool" in e
    assert e["ast_tool"]["detected"] is True
    assert e["ast_tool"]["command"] == "callers"
    assert e["ast_tool"]["raw_command"] == "ast-tool callers greet"


def test_parse_trace_events_non_ast_tool_bash(tmp_path: Path):
    entries = [
        _assistant_tool_use("id1", "Bash", {"command": "ls -la src/"}),
        _user_tool_result("id1", "main.cpp\ngreeter.h"),
    ]
    log_dir = _make_log_dir(tmp_path, entries)
    events = parse_trace_events(log_dir)

    assert len(events) == 1
    assert "ast_tool" not in events[0]


def test_parse_trace_events_timestamps_preserved(tmp_path: Path):
    entries = [
        _assistant_tool_use("id1", "Read", {"file_path": "x.cpp"}, "2026-08-26T10:00:00.000Z"),
        _user_tool_result("id1", "content", ts="2026-08-26T10:00:01.500Z"),
    ]
    log_dir = _make_log_dir(tmp_path, entries)
    events = parse_trace_events(log_dir)

    e = events[0]
    assert e.get("started_at") == "2026-08-26T10:00:00.000Z"
    assert e.get("ended_at") == "2026-08-26T10:00:01.500Z"


def test_parse_trace_events_input_output_both_present(tmp_path: Path):
    entries = [
        _assistant_tool_use("id1", "Grep", {"pattern": "greet", "path": "src/"}),
        _user_tool_result("id1", "src/main.cpp:5: greet()"),
    ]
    log_dir = _make_log_dir(tmp_path, entries)
    events = parse_trace_events(log_dir)

    e = events[0]
    assert e["input"] == {"pattern": "greet", "path": "src/"}
    assert "greet()" in e["output"]


def test_parse_trace_events_empty_log(tmp_path: Path):
    log_dir = tmp_path / "projects"
    log_dir.mkdir()
    (log_dir / "session.jsonl").write_text("", encoding="utf-8")
    events = parse_trace_events(log_dir)
    assert events == []


# ---------------------------------------------------------------------------
# write_task_trace
# ---------------------------------------------------------------------------

def _make_events(n: int) -> list[dict]:
    return [
        {
            "event": "tool_call",
            "sequence": i + 1,
            "tool": "Bash",
            "tool_use_id": f"id{i}",
            "input": {"command": f"cmd{i}"},
            "output": f"output{i}",
            "success": True,
        }
        for i in range(n)
    ]


def test_write_task_trace_creates_file(tmp_path: Path):
    trace_dir = tmp_path / "traces"
    events = _make_events(3)
    path = write_task_trace("level2-004", events, trace_dir)
    assert path.exists()
    assert path.name == "level2-004.jsonl"


def test_write_task_trace_has_task_start_end(tmp_path: Path):
    trace_dir = tmp_path / "traces"
    events = _make_events(2)
    path = write_task_trace(
        "level2-004", events, trace_dir,
        task_meta={"repository": "repos/basic-01", "agent": "claude"},
        result_meta={"success": True, "elapsed_seconds": 42.0},
    )
    lines = [json.loads(l) for l in path.read_text(encoding="utf-8").splitlines() if l.strip()]
    assert lines[0]["event"] == "task_start"
    assert lines[0]["task_id"] == "level2-004"
    assert lines[0]["repository"] == "repos/basic-01"
    assert lines[-1]["event"] == "task_end"
    assert lines[-1]["success"] is True
    assert lines[-1]["elapsed_seconds"] == 42.0


def test_write_task_trace_tool_calls_in_order(tmp_path: Path):
    trace_dir = tmp_path / "traces"
    events = _make_events(3)
    path = write_task_trace("level1-001", events, trace_dir)
    lines = [json.loads(l) for l in path.read_text(encoding="utf-8").splitlines() if l.strip()]
    tool_lines = [l for l in lines if l["event"] == "tool_call"]
    assert len(tool_lines) == 3
    for i, tl in enumerate(tool_lines):
        assert tl["sequence"] == i + 1


def test_write_task_trace_truncation(tmp_path: Path):
    trace_dir = tmp_path / "traces"
    long_output = "x" * 10000
    events = [
        {
            "event": "tool_call",
            "sequence": 1,
            "tool": "Bash",
            "tool_use_id": "id0",
            "input": {"command": "cmd"},
            "output": long_output,
            "success": True,
        }
    ]
    path = write_task_trace("level1-001", events, trace_dir, max_output_bytes=100)
    lines = [json.loads(l) for l in path.read_text(encoding="utf-8").splitlines() if l.strip()]
    tool_line = next(l for l in lines if l["event"] == "tool_call")
    assert tool_line.get("output_truncated") is True
    assert tool_line["original_output_bytes"] == 10000
    assert len(tool_line["output"].encode("utf-8")) <= 100


def test_write_task_trace_no_truncation_below_limit(tmp_path: Path):
    trace_dir = tmp_path / "traces"
    events = _make_events(1)
    path = write_task_trace("level1-001", events, trace_dir, max_output_bytes=100000)
    lines = [json.loads(l) for l in path.read_text(encoding="utf-8").splitlines() if l.strip()]
    tool_line = next(l for l in lines if l["event"] == "tool_call")
    assert "output_truncated" not in tool_line


# ---------------------------------------------------------------------------
# append_trace_index
# ---------------------------------------------------------------------------

def test_append_trace_index(tmp_path: Path):
    trace_dir = tmp_path / "traces"
    append_trace_index({"task_id": "level2-004", "tool_calls": 5}, trace_dir)
    append_trace_index({"task_id": "level2-008", "tool_calls": 3}, trace_dir)

    index_path = trace_dir / "index.jsonl"
    assert index_path.exists()
    lines = [json.loads(l) for l in index_path.read_text(encoding="utf-8").splitlines() if l.strip()]
    assert len(lines) == 2
    assert lines[0]["task_id"] == "level2-004"
    assert lines[1]["task_id"] == "level2-008"


# ---------------------------------------------------------------------------
# Trace disabled — no trace files written without trace_dir
# ---------------------------------------------------------------------------

def test_no_trace_without_trace_dir(tmp_path: Path):
    """parse_trace_events and write_task_trace are not called if trace_dir is None."""
    traces_path = tmp_path / "traces"
    assert not traces_path.exists()
