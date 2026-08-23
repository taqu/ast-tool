from pathlib import Path
from agents.base import AgentRunner, AgentRunResult
from claude import run_claude
from logs import clear_claude_logs, parse_session_logs

class ClaudeCodeRunner(AgentRunner):
    def run(self, prompt: str, repo_path: Path, timeout: int) -> AgentRunResult:
        clear_claude_logs()
        exec_result = run_claude(prompt, repo_path, timeout=timeout)
        stats = parse_session_logs()
        return AgentRunResult(
            exit_code=exec_result.exit_code,
            elapsed_seconds=exec_result.elapsed_seconds,
            timed_out=exec_result.timed_out,
            stdout=exec_result.stdout,
            stderr=exec_result.stderr,
            tokens=stats["tokens"],
            tools=stats["tools"],
            ast_tool=stats["ast_tool"],
            workflow=stats["workflow"],
        )
