import subprocess
from dataclasses import dataclass, field
from pathlib import Path


@dataclass
class ValidationResult:
    success: bool
    exit_code: int
    stdout: str
    stderr: str
    expected_files_missing: list[str] = field(default_factory=list)


def validate_task(task: dict, repo_path: Path) -> ValidationResult:
    missing: list[str] = []
    for rel in task.get("expected_files") or []:
        if not (repo_path / rel).exists():
            missing.append(rel)

    validation = task.get("validation") or {}
    command_str = validation.get("command", "")
    expected_exit = task.get("expected_exit_code", 0)

    exit_code = 0
    stdout = ""
    stderr = ""

    if command_str:
        try:
            res = subprocess.run(
                command_str,
                cwd=str(repo_path),
                shell=True,
                capture_output=True,
                text=True,
                timeout=60,
            )
            exit_code = res.returncode
            stdout = res.stdout
            stderr = res.stderr
        except subprocess.TimeoutExpired:
            exit_code = -1
            stderr = "Validation command timed out"
        except Exception as e:
            exit_code = -1
            stderr = str(e)

    success = (exit_code == expected_exit) and not missing
    return ValidationResult(
        success=success,
        exit_code=exit_code,
        stdout=stdout,
        stderr=stderr,
        expected_files_missing=missing,
    )
