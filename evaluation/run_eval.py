#!/usr/bin/env python3
"""CLI entry point: python run_eval.py tasks/smoke-001.yaml  (or a directory of YAML files)."""
import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))

from runner import run_task


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Agent Evaluation Runner — measures ast-tool usage by coding agents"
    )
    parser.add_argument(
        "tasks",
        nargs="+",
        metavar="TASK",
        help="Path to a task YAML file or a directory containing task YAML files",
    )
    parser.add_argument(
        "--agent",
        default="claude",
        metavar="AGENT",
        help="Agent to run ('claude', 'antigravity', or comma-separated list like 'claude,antigravity')",
    )
    parser.add_argument(
        "--base-dir",
        default=None,
        metavar="DIR",
        help="Base directory for resolving repository paths in tasks (default: evaluation/)",
    )
    parser.add_argument(
        "--results-dir",
        default=None,
        metavar="DIR",
        help="Directory where results.jsonl is written (default: evaluation/results/)",
    )
    args = parser.parse_args()

    eval_dir = Path(__file__).parent
    base_dir = Path(args.base_dir) if args.base_dir else eval_dir
    results_dir = Path(args.results_dir) if args.results_dir else eval_dir / "results"

    task_files: list[Path] = []
    for raw in args.tasks:
        p = Path(raw)
        if p.is_dir():
            task_files.extend(sorted(p.glob("*.yaml")))
        elif p.is_file():
            task_files.append(p)
        else:
            print(f"[run_eval] Warning: '{p}' not found, skipping")

    if not task_files:
        print("[run_eval] No task files found.")
        sys.exit(1)

    agents = [a.strip() for a in args.agent.split(",") if a.strip()]
    if not agents:
        agents = ["claude"]

    total_runs = len(task_files) * len(agents)
    print(f"[run_eval] Running {len(task_files)} task(s) for agent(s) {agents} sequentially ...")
    failures = 0
    for agent in agents:
        for task_file in task_files:
            try:
                record = run_task(task_file, base_dir, results_dir, agent_name=agent)
                status = record.get("status", "unknown")
                print(f"[run_eval] {record.get('task_id', task_file.stem)} ({agent}): {status}")
                if status != "success":
                    failures += 1
            except Exception as e:
                print(f"[run_eval] Unexpected error processing '{task_file}' with agent '{agent}': {e}")
                failures += 1

    print(f"\n[run_eval] {total_runs - failures}/{total_runs} runs succeeded.")
    sys.exit(0 if failures == 0 else 1)


if __name__ == "__main__":
    main()
