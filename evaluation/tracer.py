#!/usr/bin/env python3
"""Detailed per-tool-call trace capture for evaluation runs."""
from __future__ import annotations

import glob
import json
import re
from pathlib import Path
from typing import Any

from logs import CLAUDE_LOG_DIR

_AST_TOOL_RE = re.compile(r'\bast-tool\s+(\w+)(.*)')


# ---------------------------------------------------------------------------
# ast-tool detection
# ---------------------------------------------------------------------------

def detect_ast_tool(tool_name: str, tool_input: Any) -> dict[str, Any] | None:
    """
    Return ast-tool metadata if the tool call invokes ast-tool, else None.

    Always preserves raw_command even when argument parsing is applied.
    """
    if tool_name != "Bash":
        return None

    cmd = (
        tool_input.get("command", "")
        if isinstance(tool_input, dict)
        else str(tool_input)
    )

    m = _AST_TOOL_RE.search(cmd)
    if not m:
        return None

    return {
        "detected": True,
        "command": m.group(1),
        "raw_command": cmd,
    }


# ---------------------------------------------------------------------------
# Log parsing
# ---------------------------------------------------------------------------

def _read_log_entries(log_dir: Path) -> list[dict[str, Any]]:
    jsonl_files = sorted(
        glob.glob(str(log_dir / "**/*.jsonl"), recursive=True),
        key=lambda p: Path(p).stat().st_mtime,
    )

    entries: list[dict[str, Any]] = []

    for file_path in jsonl_files:
        try:
            with open(file_path, "r", encoding="utf-8") as f:
                for line in f:
                    if not line.strip():
                        continue
                    try:
                        entries.append(json.loads(line))
                    except json.JSONDecodeError:
                        pass
        except Exception as e:
            print(f"[tracer] Warning: could not read {file_path}: {e}")

    return entries


def _extract_output_text(content: Any) -> str:
    """Normalize tool_result content to a plain string."""
    if content is None:
        return ""
    if isinstance(content, str):
        return content
    if isinstance(content, list):
        parts: list[str] = []
        for block in content:
            if isinstance(block, dict):
                parts.append(block.get("text", str(block)))
            else:
                parts.append(str(block))
        return "\n".join(parts)
    return str(content)


def parse_trace_events(log_dir: Path | None = None) -> list[dict[str, Any]]:
    """
    Parse Claude Code session logs and return ordered trace events.

    Each event combines a tool_use (tool name + input) with its tool_result
    (output + success flag).  Events are numbered sequentially starting at 1.

    Returns a list of dicts with keys:
        event       "tool_call"
        sequence    int
        tool        str (tool name)
        tool_use_id str
        input       dict
        output      str (raw text)
        success     bool
        ast_tool    dict | None
        started_at  str | None  (ISO timestamp from assistant entry)
        ended_at    str | None  (ISO timestamp from user entry)
    """
    if log_dir is None:
        log_dir = CLAUDE_LOG_DIR

    entries = _read_log_entries(log_dir)

    # First pass: collect tool_use and tool_result events keyed by tool_use_id.
    # Maps tool_use_id → {name, input, started_at}
    pending: dict[str, dict[str, Any]] = {}

    # Ordered list of completed (tool_use_id, result) pairs preserving emission
    # order (which matches assistant-message order).
    order: list[str] = []
    results: dict[str, dict[str, Any]] = {}

    for entry in entries:
        entry_type = entry.get("type")
        ts = entry.get("timestamp")
        msg = entry.get("message") or {}
        content_list = msg.get("content") or []

        if entry_type == "assistant":
            for block in content_list:
                if not isinstance(block, dict):
                    continue
                if block.get("type") != "tool_use":
                    continue
                uid = block.get("id")
                if not uid:
                    continue
                pending[uid] = {
                    "name": block.get("name", ""),
                    "input": block.get("input") or {},
                    "started_at": ts,
                }
                order.append(uid)

        elif entry_type == "user":
            for block in content_list:
                if not isinstance(block, dict):
                    continue
                if block.get("type") != "tool_result":
                    continue
                uid = block.get("tool_use_id")
                if not uid:
                    continue
                results[uid] = {
                    "content": block.get("content"),
                    "is_error": bool(block.get("is_error", False)),
                    "ended_at": ts,
                }

    # Second pass: build ordered trace events.
    events: list[dict[str, Any]] = []
    seq = 0

    for uid in order:
        pending_entry = pending.get(uid)
        if pending_entry is None:
            continue

        tool_name = pending_entry["name"]
        tool_input = pending_entry["input"]
        started_at = pending_entry["started_at"]

        result_entry = results.get(uid)
        if result_entry:
            raw_output = _extract_output_text(result_entry["content"])
            is_error = result_entry["is_error"]
            ended_at = result_entry["ended_at"]
        else:
            raw_output = ""
            is_error = False
            ended_at = None

        ast_meta = detect_ast_tool(tool_name, tool_input)

        seq += 1
        event: dict[str, Any] = {
            "event": "tool_call",
            "sequence": seq,
            "tool": tool_name,
            "tool_use_id": uid,
            "input": tool_input,
            "output": raw_output,
            "success": not is_error,
        }

        if ast_meta:
            event["ast_tool"] = ast_meta

        if started_at:
            event["started_at"] = started_at

        if ended_at:
            event["ended_at"] = ended_at

        events.append(event)

    return events


# ---------------------------------------------------------------------------
# Trace file writing
# ---------------------------------------------------------------------------

def _maybe_truncate(
    output: str,
    max_bytes: int | None,
) -> tuple[str, bool, int]:
    """Return (output, truncated, original_bytes)."""
    if max_bytes is None:
        return output, False, len(output.encode("utf-8"))

    encoded = output.encode("utf-8")
    original = len(encoded)

    if original <= max_bytes:
        return output, False, original

    truncated = encoded[:max_bytes].decode("utf-8", errors="ignore")
    return truncated, True, original


def write_task_trace(
    task_id: str,
    events: list[dict[str, Any]],
    trace_dir: Path,
    task_meta: dict[str, Any] | None = None,
    result_meta: dict[str, Any] | None = None,
    max_output_bytes: int | None = None,
) -> Path:
    """
    Write a JSONL trace file for one task.

    Returns the path of the written file.
    """
    trace_dir.mkdir(parents=True, exist_ok=True)
    trace_path = trace_dir / f"{task_id}.jsonl"

    with trace_path.open("w", encoding="utf-8") as f:
        # task_start
        start_event: dict[str, Any] = {
            "event": "task_start",
            "task_id": task_id,
        }
        if task_meta:
            start_event.update(task_meta)
        f.write(json.dumps(start_event) + "\n")

        # tool_call events
        for event in events:
            ev = dict(event)

            output = ev.get("output", "")
            if isinstance(output, str):
                truncated_output, was_truncated, orig_bytes = _maybe_truncate(
                    output, max_output_bytes
                )
                ev["output"] = truncated_output
                if was_truncated:
                    ev["output_truncated"] = True
                    ev["original_output_bytes"] = orig_bytes

            f.write(json.dumps(ev) + "\n")

        # task_end
        end_event: dict[str, Any] = {
            "event": "task_end",
            "task_id": task_id,
            "tool_calls": len(events),
        }
        if result_meta:
            end_event.update(result_meta)
        f.write(json.dumps(end_event) + "\n")

    return trace_path


def append_trace_index(
    entry: dict[str, Any],
    trace_dir: Path,
) -> None:
    """Append one entry to the trace index file."""
    trace_dir.mkdir(parents=True, exist_ok=True)
    index_path = trace_dir / "index.jsonl"

    with index_path.open("a", encoding="utf-8") as f:
        f.write(json.dumps(entry) + "\n")
