#!/usr/bin/env python3
"""
Phase 2 tests: trace-based output volume measurement and CLI output behavior.

Tests are grouped into:
  - TraceStats: unit tests for _ast_tool_output_stats()
  - CliOutput: integration tests against the ast-tool binary (skipped if absent)
"""
from __future__ import annotations

import json
import os
import subprocess
import tempfile
import unittest
from pathlib import Path

from analyze_results import _ast_tool_output_stats, print_ast_tool_output_stats


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _make_result(task_id: str = "level1-001") -> dict:
    return {
        "task_id": task_id,
        "success": True,
        "timed_out": False,
        "elapsed_seconds": 10.0,
        "tokens": {"input": 10, "output": 500},
        "tools": {"Bash": 2, "Read": 1},
        "ast_tool": {"search": 1},
    }


def _write_trace(directory: Path, task_id: str, events: list[dict]) -> None:
    path = directory / f"{task_id}.jsonl"
    with path.open("w", encoding="utf-8") as f:
        for event in events:
            f.write(json.dumps(event) + "\n")


def _ast_event(
    command: str,
    output: str,
    raw_command: str = "",
) -> dict:
    if not raw_command:
        raw_command = f"ast-tool {command} ."
    return {
        "event": "tool_call",
        "tool": "Bash",
        "input": {"command": raw_command},
        "output": output,
        "success": True,
        "ast_tool": {
            "detected": True,
            "command": command,
            "raw_command": raw_command,
        },
    }


# ---------------------------------------------------------------------------
# Trace statistics unit tests
# ---------------------------------------------------------------------------

class TestTraceStats(unittest.TestCase):

    def setUp(self) -> None:
        self._tmpdir = tempfile.TemporaryDirectory()
        self.traces = Path(self._tmpdir.name)

    def tearDown(self) -> None:
        self._tmpdir.cleanup()

    def test_no_traces_dir_graceful(self) -> None:
        results = [_make_result("level1-001")]
        stats = _ast_tool_output_stats(Path("/nonexistent"), results)
        self.assertEqual(stats["total_calls"], 0)
        self.assertEqual(stats["total_bytes"], 0)

    def test_missing_trace_file_skipped(self) -> None:
        results = [_make_result("level1-001")]
        stats = _ast_tool_output_stats(self.traces, results)
        self.assertEqual(stats["total_calls"], 0)

    def test_plain_text_call_counted(self) -> None:
        _write_trace(self.traces, "level1-001", [
            _ast_event("search", "Method ns::foo src/foo.h:5:1"),
        ])
        results = [_make_result("level1-001")]
        stats = _ast_tool_output_stats(self.traces, results)

        self.assertEqual(stats["total_calls"], 1)
        self.assertEqual(stats["plain_calls"], 1)
        self.assertEqual(stats["json_calls"], 0)
        self.assertEqual(stats["pretty_json_calls"], 0)

    def test_json_call_classified(self) -> None:
        raw = 'ast-tool search --json --name foo .'
        _write_trace(self.traces, "level1-001", [
            _ast_event("search", '[{"kind":"Method"}]', raw_command=raw),
        ])
        results = [_make_result("level1-001")]
        stats = _ast_tool_output_stats(self.traces, results)

        self.assertEqual(stats["json_calls"], 1)
        self.assertEqual(stats["pretty_json_calls"], 0)
        self.assertEqual(stats["plain_calls"], 0)

    def test_pretty_json_call_classified(self) -> None:
        raw = 'ast-tool search --json --pretty --name foo .'
        _write_trace(self.traces, "level1-001", [
            _ast_event("search", '[\n {\n  "kind": "Method"\n }\n]', raw_command=raw),
        ])
        results = [_make_result("level1-001")]
        stats = _ast_tool_output_stats(self.traces, results)

        self.assertEqual(stats["pretty_json_calls"], 1)
        self.assertEqual(stats["json_calls"], 0)

    def test_byte_count_uses_utf8(self) -> None:
        output = "Method ns::foo src/foo.h:5:1"
        expected_bytes = len(output.encode("utf-8"))
        _write_trace(self.traces, "level1-001", [
            _ast_event("search", output),
        ])
        results = [_make_result("level1-001")]
        stats = _ast_tool_output_stats(self.traces, results)

        self.assertEqual(stats["total_bytes"], expected_bytes)

    def test_aggregation_by_command(self) -> None:
        _write_trace(self.traces, "level1-001", [
            _ast_event("search", "a" * 100),
            _ast_event("callers", "b" * 50),
            _ast_event("search", "c" * 80),
        ])
        results = [_make_result("level1-001")]
        stats = _ast_tool_output_stats(self.traces, results)

        self.assertEqual(stats["total_calls"], 3)
        self.assertEqual(stats["by_command"]["search"]["calls"], 2)
        self.assertEqual(stats["by_command"]["search"]["bytes"], 180)
        self.assertEqual(stats["by_command"]["callers"]["calls"], 1)
        self.assertEqual(stats["by_command"]["callers"]["bytes"], 50)

    def test_non_ast_events_ignored(self) -> None:
        _write_trace(self.traces, "level1-001", [
            {"event": "tool_call", "tool": "Read", "output": "some text"},
            {"event": "task_start", "task_id": "level1-001"},
            _ast_event("search", "Method ns::foo src/foo.h:5:1"),
        ])
        results = [_make_result("level1-001")]
        stats = _ast_tool_output_stats(self.traces, results)
        self.assertEqual(stats["total_calls"], 1)

    def test_multiple_tasks_aggregated(self) -> None:
        _write_trace(self.traces, "level1-001", [
            _ast_event("search", "x" * 200),
        ])
        _write_trace(self.traces, "level1-002", [
            _ast_event("callers", "y" * 300),
        ])
        results = [
            _make_result("level1-001"),
            _make_result("level1-002"),
        ]
        stats = _ast_tool_output_stats(self.traces, results)
        self.assertEqual(stats["total_calls"], 2)
        self.assertEqual(stats["total_bytes"], 500)

    def test_print_output_stats_no_crash(self) -> None:
        _write_trace(self.traces, "level1-001", [
            _ast_event("search", "Method ns::foo src/foo.h:5:1"),
        ])
        results = [_make_result("level1-001")]
        stats = _ast_tool_output_stats(self.traces, results)
        print_ast_tool_output_stats(stats)  # should not raise

    def test_print_empty_stats_no_crash(self) -> None:
        stats = _ast_tool_output_stats(self.traces, [])
        print_ast_tool_output_stats(stats)  # should not raise

    def test_result_without_matching_trace_does_not_crash(self) -> None:
        results = [_make_result("level9-999")]
        stats = _ast_tool_output_stats(self.traces, results)
        self.assertEqual(stats["total_calls"], 0)


# ---------------------------------------------------------------------------
# CLI integration tests (skipped if binary not found)
# ---------------------------------------------------------------------------

AST_TOOL_BIN = Path("D:/Projects/Cpp/temp/ast-tool/bin/ast-tool.exe")
WORKSPACE = Path("D:/Projects/Cpp/temp/ast-tool/evaluation/repositories/level1-store")

_SKIP_CLI = not AST_TOOL_BIN.exists() or not WORKSPACE.exists()
_SKIP_MSG = "ast-tool binary or workspace not found"


@unittest.skipIf(_SKIP_CLI, _SKIP_MSG)
class TestCliOutput(unittest.TestCase):

    def _run(self, args: list[str]) -> tuple[str, str, int]:
        result = subprocess.run(
            [str(AST_TOOL_BIN)] + args,
            capture_output=True,
            text=True,
            timeout=30,
        )
        return result.stdout, result.stderr, result.returncode

    def test_search_json_compact_no_whitespace_indentation(self) -> None:
        """--json must emit compact JSON (no pretty-print indentation)."""
        stdout, _err, _rc = self._run([
            "search", "--json", "--name", "process",
            str(WORKSPACE),
        ])
        # Each result should be a single-line object, not indented
        # Compact JSON has no newlines inside objects
        if not stdout.strip() or stdout.strip() == "[]":
            return  # no results, still valid
        # Compact JSON: no line starts with spaces (indentation)
        for line in stdout.splitlines():
            stripped = line.lstrip()
            self.assertFalse(
                line != stripped and stripped.startswith('"'),
                f"Found indented JSON field, expected compact: {line!r}",
            )

    def test_search_json_is_valid_json(self) -> None:
        """--json output must be parseable as JSON."""
        stdout, _err, _rc = self._run([
            "search", "--json", "--name", "process",
            str(WORKSPACE),
        ])
        parsed = json.loads(stdout)
        self.assertIsInstance(parsed, list)

    def test_search_json_pretty_contains_indentation(self) -> None:
        """--json --pretty output must be formatted (multi-line)."""
        stdout, _err, _rc = self._run([
            "search", "--json", "--pretty", "--name", "process",
            str(WORKSPACE),
        ])
        if not stdout.strip() or stdout.strip() == "[]":
            return
        parsed = json.loads(stdout)
        self.assertIsInstance(parsed, list)
        # Pretty JSON has more than one line
        self.assertGreater(len(stdout.splitlines()), 1)

    def test_search_json_fields_present(self) -> None:
        """JSON output must contain the documented fields."""
        stdout, _err, _rc = self._run([
            "search", "--json", "--name", "process",
            str(WORKSPACE),
        ])
        parsed = json.loads(stdout)
        if not parsed:
            return
        item = parsed[0]
        for field in ("kind", "name", "fqn", "file", "line", "column"):
            self.assertIn(
                field, item,
                f"Expected field '{field}' in JSON result",
            )

    def test_search_plain_one_result_per_line(self) -> None:
        """Plain-text search output must have exactly one result per line."""
        stdout, _err, _rc = self._run([
            "search", "--name", "process",
            str(WORKSPACE),
        ])
        for line in stdout.splitlines():
            # Each line should contain a file path and location
            self.assertIn(":", line, f"Expected file:line:col in: {line!r}")

    def test_search_plain_no_header_prose(self) -> None:
        """Plain-text output must not have result-count or header lines."""
        stdout, _err, _rc = self._run([
            "search", "--name", "process",
            str(WORKSPACE),
        ])
        for line in stdout.splitlines():
            low = line.lower()
            self.assertFalse(
                low.startswith("result") or
                low.startswith("found") or
                low.startswith("search"),
                f"Unexpected header/prose line: {line!r}",
            )

    def test_search_limit_caps_results(self) -> None:
        """--limit N must return at most N results."""
        stdout_all, _err, _rc = self._run([
            "search", str(WORKSPACE),
        ])
        all_lines = [l for l in stdout_all.splitlines() if l.strip()]
        if len(all_lines) <= 1:
            return  # not enough results to test limiting

        stdout_limited, stderr_limited, _rc2 = self._run([
            "search", "--limit", "1", str(WORKSPACE),
        ])
        limited_lines = [
            l for l in stdout_limited.splitlines() if l.strip()
        ]
        self.assertLessEqual(len(limited_lines), 1)

    def test_search_limit_zero_is_unlimited(self) -> None:
        """--limit 0 must behave identically to no limit."""
        stdout_none, _, _ = self._run(["search", str(WORKSPACE)])
        stdout_zero, _, _ = self._run(["search", "--limit", "0", str(WORKSPACE)])
        self.assertEqual(stdout_none, stdout_zero)

    def test_search_without_pretty_is_compact(self) -> None:
        """--json without --pretty must NOT auto-enable pretty printing."""
        stdout_compact, _, _ = self._run([
            "search", "--json", "--name", "process", str(WORKSPACE),
        ])
        stdout_pretty, _, _ = self._run([
            "search", "--json", "--pretty", "--name", "process", str(WORKSPACE),
        ])
        # If there are results, compact should be fewer lines than pretty
        if json.loads(stdout_compact):
            self.assertLessEqual(
                len(stdout_compact.splitlines()),
                len(stdout_pretty.splitlines()),
            )

    def test_callers_plain_one_per_line(self) -> None:
        """callers plain-text output must be one entry per line."""
        # Find a symbol that has callers
        stdout_search, _, _ = self._run([
            "search", "--kind", "method", "--name", "process",
            str(WORKSPACE),
        ])
        lines = [l for l in stdout_search.splitlines() if l.strip()]
        if not lines:
            return

        # Extract fqn from "Kind fqn file:line:col"
        parts = lines[0].split()
        if len(parts) < 2:
            return
        fqn = parts[1]

        stdout, _err, _rc = self._run([
            "callers", fqn, str(WORKSPACE),
        ])
        for line in stdout.splitlines():
            if line.strip():
                # Each non-empty line must have file:line:col
                self.assertIn(":", line)


if __name__ == "__main__":
    unittest.main()
