#!/usr/bin/env python3
"""Initialize git repositories in evaluation/repositories/ from fixtures."""
import os
import shutil
import subprocess
import sys
from pathlib import Path

EVAL_DIR = Path(__file__).parent
FIXTURES_DIR = EVAL_DIR / "fixtures"
REPOS_DIR = EVAL_DIR / "repositories"

_GIT_ENV = {
    **os.environ,
    "GIT_AUTHOR_NAME": "eval-setup",
    "GIT_AUTHOR_EMAIL": "eval-setup@local",
    "GIT_COMMITTER_NAME": "eval-setup",
    "GIT_COMMITTER_EMAIL": "eval-setup@local",
}


def setup_repo(fixture_name: str) -> None:
    src = FIXTURES_DIR / fixture_name
    dst = REPOS_DIR / fixture_name

    if not src.exists():
        print(f"[setup] Fixture not found: {src}")
        return

    if dst.exists():
        shutil.rmtree(dst)

    shutil.copytree(src, dst)

    def git(*args: str) -> None:
        subprocess.run(["git", *args], cwd=str(dst), check=True, capture_output=True, env=_GIT_ENV)

    git("init")
    git("add", "-A")
    git("commit", "-m", "initial")
    print(f"[setup] Initialized {dst}")


def main() -> None:
    REPOS_DIR.mkdir(parents=True, exist_ok=True)

    if not FIXTURES_DIR.exists():
        print(f"[setup] No fixtures directory found at {FIXTURES_DIR}")
        sys.exit(1)

    fixtures = [d.name for d in FIXTURES_DIR.iterdir() if d.is_dir()]
    if not fixtures:
        print("[setup] No fixture directories found.")
        return

    for name in sorted(fixtures):
        setup_repo(name)

    print("[setup] Done.")


if __name__ == "__main__":
    main()
