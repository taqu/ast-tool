import os
import shutil
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path


@dataclass
class ExecutionResult:
    exit_code: int
    elapsed_seconds: float
    timed_out: bool
    stdout: str
    stderr: str


def _find_claude_exe() -> str:
    candidates = ["claude", "claude.cmd"] if sys.platform == "win32" else ["claude"]
    for name in candidates:
        if shutil.which(name):
            return name
    return "claude"


def run_claude(prompt: str, cwd: Path, timeout: int = 300) -> ExecutionResult:
    exe = _find_claude_exe()
    env = os.environ.copy()
    env["CI"] = "true"
    env["PYTHONUNBUFFERED"] = "1"

    command = [exe, "--dangerously-skip-permissions", "-p", prompt]

    start = time.time()
    timed_out = False
    stdout_data = ""
    stderr_data = ""
    exit_code = -1

    try:
        proc = subprocess.Popen(
            command,
            cwd=str(cwd),
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        try:
            stdout_data, stderr_data = proc.communicate(timeout=timeout)
            exit_code = proc.returncode
        except subprocess.TimeoutExpired:
            proc.kill()
            stdout_data, stderr_data = proc.communicate()
            timed_out = True
            exit_code = -1
    except FileNotFoundError:
        stderr_data = (
            f"Error: '{exe}' not found. "
            "Install Claude Code: npm install -g @anthropic-ai/claude-code"
        )
        exit_code = -2

    return ExecutionResult(
        exit_code=exit_code,
        elapsed_seconds=round(time.time() - start, 2),
        timed_out=timed_out,
        stdout=stdout_data,
        stderr=stderr_data,
    )
