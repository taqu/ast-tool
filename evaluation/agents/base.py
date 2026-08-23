from dataclasses import dataclass
from pathlib import Path

@dataclass
class AgentRunResult:
    exit_code: int
    elapsed_seconds: float
    timed_out: bool
    stdout: str
    stderr: str
    tokens: dict
    tools: dict
    ast_tool: dict
    workflow: list

class AgentRunner:
    def run(self, prompt: str, repo_path: Path, timeout: int) -> AgentRunResult:
        raise NotImplementedError()
