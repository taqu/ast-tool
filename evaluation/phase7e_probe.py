"""Repeat unchanged normal tasks, preserving existing Claude session logs."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import re
import argparse

import logs
import agents.claude_code as adapter
from runner import run_task
from trace_analyzer import analyze_directory

ROOT = Path(__file__).resolve().parent
OUT = ROOT / 'phase7e' / 'normal'
COHORT = ['level1-001', 'level2-006', 'level2-008', 'level3-003',
          'level3-007', 'level3-008', 'level4-003', 'smoke-001']
EXPECTED = '96b07a6b89ae338f26d45fbfb31dd97d5b9c50efa39922103e8cb3e616807eaf'
SKILLS = [ROOT.parent / 'skills/semantic-analysis/SKILL.md',
          Path.home() / '.claude/skills/semantic-analysis/SKILL.md']
GLOBAL_LOGS = Path.home() / '.claude/projects'
original_run = adapter.run_claude

def hashes():
    result = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in SKILLS}
    assert all(v == EXPECTED for v in result.values()), result
    return result

def isolated_run(prompt, cwd, timeout):
    project = GLOBAL_LOGS / re.sub(r'[^a-zA-Z0-9]', '-', str(cwd))
    before = set(project.rglob('*.jsonl')) if project.exists() else set()
    result = original_run(prompt, cwd, timeout)
    fresh = set(project.rglob('*.jsonl')) - before
    for p in fresh:
        target = logs.CLAUDE_LOG_DIR / p.relative_to(project)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(p, target)
    (logs.CLAUDE_LOG_DIR / 'process.json').write_text(json.dumps({
        'exit_code': result.exit_code, 'stderr': result.stderr,
        'stdout': result.stdout, 'fresh_logs': len(fresh)}, indent=2), encoding='utf-8')
    return result

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--first-repeat', type=int, default=1)
    parser.add_argument('--last-repeat', type=int, default=5)
    parser.add_argument('--task', action='append')
    args = parser.parse_args()
    assert os.environ.get('AST_TOOL_CONTROLLED_SKILL') != '1'
    OUT.mkdir(parents=True, exist_ok=True)
    manifest = {'revision': subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip(),
                'cohort': COHORT, 'forced': False, 'before': hashes(), 'checks': []}
    if (OUT / 'manifest.json').exists():
        manifest = json.loads((OUT / 'manifest.json').read_text(encoding='utf-8'))
    adapter.clear_claude_logs = lambda: None
    adapter.run_claude = isolated_run
    for repeat in range(args.first_repeat, args.last_repeat + 1):
        dest = OUT / f'r{repeat}'
        offset = (repeat - 1) % len(COHORT)
        for task in COHORT[offset:] + COHORT[:offset]:
            if args.task and task not in args.task:
                continue
            if (dest / 'analysis' / f'{task}.json').exists():
                continue
            hashes()
            logs.CLAUDE_LOG_DIR = dest / 'sessions' / task
            logs.CLAUDE_LOG_DIR.mkdir(parents=True, exist_ok=True)
            record = run_task(ROOT / 'tasks' / f'{task}.yaml', ROOT, dest,
                              trace_dir=dest / 'traces')
            manifest['checks'].append({'repeat': repeat, 'task': task, 'after': hashes()})
            (OUT / 'manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
            summaries = analyze_directory(dest / 'traces', results_path=dest / 'results.jsonl')
            (dest / 'analysis').mkdir(exist_ok=True)
            for row in summaries:
                (dest / 'analysis' / (row['task_id'] + '.json')).write_text(json.dumps(row, indent=2), encoding='utf-8')
            print(f"PROBE {repeat} {task}: {record['status']}", flush=True)
            if record['status'] in ('runner_failure', 'agent_process_failure') or not record.get('tools'):
                raise RuntimeError('Infrastructure failure; inspect captured process.json before continuing')

if __name__ == '__main__':
    main()
