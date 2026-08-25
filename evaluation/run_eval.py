#!/usr/bin/env python3
"""CLI entry point: python run_eval.py tasks/smoke-001.yaml  (or a directory of YAML files)."""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))

from runner import run_task, load_task, load_latest_results, should_run_task


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
    mode_group = parser.add_mutually_exclusive_group()
    mode_group.add_argument(
        "--resume",
        action="store_true",
        help="Resume incomplete evaluation (default behavior)",
    )
    mode_group.add_argument(
        "--retry-failed",
        action="store_true",
        help="Rerun only previously failed tasks",
    )
    mode_group.add_argument(
        "--force",
        action="store_true",
        help="Rerun all selected tasks regardless of previous results",
    )
    mode_group.add_argument(
        "--timeout",
        default=-1,
        type=int,
        help="Overwrite the timeout for each task (in seconds)",
    )
    parser.add_argument(
        "--task",
        action="append",
        dest="task_ids",
        metavar="TASK_ID",
        help="Run only this task ID (can be repeated). e.g. --task level2-004",
    )
    parser.add_argument(
        "--trace-tools",
        action="store_true",
        help="Write detailed per-tool-call trace files alongside results",
    )
    parser.add_argument(
        "--trace-dir",
        default=None,
        metavar="DIR",
        help="Directory for trace files (default: <results-dir>/traces/)",
    )
    parser.add_argument(
        "--max-trace-output-bytes",
        default=None,
        type=int,
        metavar="N",
        help="Truncate tool outputs in traces to at most N bytes",
    )
    args = parser.parse_args()

    eval_dir = Path(__file__).parent
    base_dir = Path(args.base_dir) if args.base_dir else eval_dir
    results_dir = Path(args.results_dir) if args.results_dir else eval_dir / "results"
    timeout = args.timeout

    trace_dir: Path | None = None
    if args.trace_tools:
        trace_dir = (
            Path(args.trace_dir)
            if args.trace_dir
            else results_dir / "traces"
        )

    task_id_filter: set[str] | None = (
        set(args.task_ids) if args.task_ids else None
    )

    if args.retry_failed:
        mode = "retry-failed"
    elif args.force:
        mode = "force"
    else:
        mode = "resume"

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

    tasks_with_ids: list[tuple[Path, str]] = []
    for task_file in task_files:
        try:
            task = load_task(task_file)
            task_id = task["id"]
            if task_id_filter is not None and task_id not in task_id_filter:
                continue
            tasks_with_ids.append((task_file, task_id))
        except Exception as e:
            print(f"[run_eval] Error loading task {task_file}: {e}")

    if not tasks_with_ids:
        print("[run_eval] No valid task files found.")
        if task_id_filter:
            print(f"[run_eval] Task ID filter was: {sorted(task_id_filter)}")
        sys.exit(1)

    agents = [a.strip() for a in args.agent.split(",") if a.strip()]
    if not agents:
        agents = ["claude"]

    results_path = results_dir / "results.jsonl"
    latest_results = load_latest_results(results_path)

    # Selection summary and counts calculation
    runs_to_execute = []
    skipped_runs = []

    for agent in agents:
        passed_cnt = 0
        failed_cnt = 0
        missing_cnt = 0
        selected_cnt = 0
        skipped_cnt = 0

        agent_runs = []
        agent_skipped = []

        for task_file, task_id in tasks_with_ids:
            previous = latest_results.get((agent, task_id))
            if previous is None:
                missing_cnt += 1
            elif previous.get("success", False):
                passed_cnt += 1
            else:
                failed_cnt += 1

            if should_run_task(task_id, agent, latest_results, mode):
                selected_cnt += 1
                agent_runs.append((task_file, task_id))
            else:
                skipped_cnt += 1
                agent_skipped.append((task_file, task_id))

        # Print selection summary for this agent
        print("Evaluation selection\n")
        print(f"Agent: {agent}")
        print(f"Mode: {mode}")
        print()
        if mode == "retry-failed":
            print(f"{'Total tasks:':<18}{len(tasks_with_ids):>3}")
            print(f"{'Previously failed:':<18}{failed_cnt:>3}")
            print(f"{'Selected to run:':<18}{selected_cnt:>3}")
            print(f"{'Skipped:':<18}{skipped_cnt:>3}")
        elif mode == "force":
            print(f"{'Total tasks:':<17}{len(tasks_with_ids):>3}")
            print(f"{'Selected to run:':<17}{selected_cnt:>3}")
            print(f"{'Skipped:':<17}{skipped_cnt:>3}")
        else: # resume
            print(f"{'Total tasks:':<17}{len(tasks_with_ids):>3}")
            print(f"{'Already passed:':<17}{passed_cnt:>3}")
            print(f"{'Failed / retry:':<17}{failed_cnt:>3}")
            print(f"{'Missing result:':<17}{missing_cnt:>3}")
            print(f"{'Selected to run:':<17}{selected_cnt:>3}")
            print(f"{'Skipped:':<17}{skipped_cnt:>3}")
        print()

        runs_to_execute.extend([(agent, tf, tid) for tf, tid in agent_runs])
        skipped_runs.extend([(agent, tf, tid) for tf, tid in agent_skipped])

    total_runs = len(runs_to_execute) + len(skipped_runs)
    print(f"[run_eval] Running {len(runs_to_execute)} run(s) sequentially ...")
    failures = 0

    for agent in agents:
        for task_file, task_id in tasks_with_ids:
            if not should_run_task(task_id, agent, latest_results, mode):
                print(f"[run_eval] {task_id} ({agent}): skipped")
                continue

            try:
                record = run_task(
                    task_file, base_dir, results_dir,
                    agent_name=agent,
                    overwrite_timeout=timeout,
                    trace_dir=trace_dir,
                    max_trace_output_bytes=args.max_trace_output_bytes,
                )
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
