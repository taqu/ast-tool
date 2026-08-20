import glob
import json
import re
import shutil
from collections import defaultdict
from pathlib import Path

CLAUDE_LOG_DIR = Path("~/.claude/projects").expanduser()
_AST_TOOL_RE = re.compile(r'\bast-tool\s+(\w+)')


def clear_claude_logs() -> None:
    if CLAUDE_LOG_DIR.exists():
        shutil.rmtree(CLAUDE_LOG_DIR)
    CLAUDE_LOG_DIR.mkdir(parents=True, exist_ok=True)


def _detect_ast_tool_cmd(tool_name: str, tool_input) -> str | None:
    if tool_name != "Bash":
        return None
    cmd = tool_input.get("command", "") if isinstance(tool_input, dict) else str(tool_input)
    m = _AST_TOOL_RE.search(cmd)
    return m.group(1) if m else None


def parse_session_logs() -> dict:
    """Aggregate token usage, tool counts, ast-tool subcommands, and ordered workflow from Claude Code logs."""
    jsonl_files = sorted(
        glob.glob(str(CLAUDE_LOG_DIR / "**/*.jsonl"), recursive=True),
        key=lambda p: Path(p).stat().st_mtime,
    )

    tokens: dict[str, int] = {"input": 0, "output": 0, "cache_read": 0, "cache_creation": 0}
    tools: dict[str, int] = defaultdict(int)
    ast_tool: dict[str, int] = defaultdict(int)
    workflow: list[dict] = []

    for file_path in jsonl_files:
        try:
            with open(file_path, "r", encoding="utf-8") as f:
                for line in f:
                    if not line.strip():
                        continue
                    data = json.loads(line)
                    message = data.get("message")
                    if not message:
                        continue

                    usage = message.get("usage") or message.get("token_usage")
                    if usage:
                        tokens["input"] += usage.get("input_tokens", 0)
                        tokens["output"] += usage.get("output_tokens", 0)
                        tokens["cache_read"] += usage.get("cache_read_tokens", 0)
                        tokens["cache_creation"] += usage.get("cache_creation_tokens", 0)

                    for content in message.get("content") or []:
                        if not isinstance(content, dict):
                            continue
                        if content.get("type") != "tool_use":
                            continue
                        tool_name = content.get("name")
                        if not tool_name:
                            continue
                        tools[tool_name] += 1
                        ast_cmd = _detect_ast_tool_cmd(tool_name, content.get("input", {}))
                        if ast_cmd:
                            ast_tool[ast_cmd] += 1
                        event: dict = {"tool": tool_name}
                        if ast_cmd:
                            event["ast_tool_command"] = ast_cmd
                        workflow.append(event)
        except Exception as e:
            print(f"[logs] Warning: could not parse {file_path}: {e}")

    return {
        "tokens": tokens,
        "tools": dict(tools),
        "ast_tool": dict(ast_tool),
        "workflow": workflow,
    }
