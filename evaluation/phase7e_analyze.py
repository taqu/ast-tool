"""Export invocation measurements separately from interpretation."""
import json
from pathlib import Path
from statistics import mean, median
from collections import Counter
import yaml
import hashlib
import subprocess
from trace_analyzer import analyze_directory, _get_ast_meta, _is_help_invocation

ROOT = Path(__file__).resolve().parent
OUT = ROOT / 'phase7e'

def collect(directory, arm, trace_dir=None, results_path=None):
    trace_dir = trace_dir or directory / 'traces'
    results_path = results_path or directory / 'results.jsonl'
    rows = []
    for summary in analyze_directory(trace_dir, results_path=results_path):
        task = summary['task_id']
        events = [json.loads(s) for s in (trace_dir / f'{task}.jsonl').read_text(encoding='utf-8').splitlines()]
        events = [e for e in events if e.get('event') == 'tool_call']
        assert len(events) == summary['total_tool_calls']
        assert len({e['tool_use_id'] for e in events}) == len(events)
        original_ids = [e['tool_use_id'] for e in events]
        events.sort(key=lambda e: (e.get('started_at') or '', e.get('sequence', 0)))
        chronological = [('ast-tool:' + e['ast_tool']['command']) if e.get('ast_tool') else e['tool'] for e in events]
        recovered = []
        failed_commands = set()
        chronological_retries = 0
        for i, event in enumerate(events):
            meta = _get_ast_meta(event)
            if not meta:
                continue
            chronological_retries += int(meta['command'] in failed_commands)
            if event.get('success', True):
                continue
            failed_commands.add(meta['command'])
            for j in range(i+1, len(events)):
                following = _get_ast_meta(events[j])
                if following and events[j].get('success', True) and following['command'] != 'top_level' and not _is_help_invocation(following.get('raw_command', '')):
                    recovered.append(j-i)
                    break
        positions = [i for i,e in enumerate(events) if e.get('tool') == 'Skill' and e.get('input', {}).get('skill', '').split(':')[-1] == 'semantic-analysis' and e.get('success') is not False]
        first = positions[0] if positions else None
        before = events[:first] if first is not None else events
        task_data = yaml.safe_load((ROOT / 'tasks' / f'{task}.yaml').read_text(encoding='utf-8'))
        fixture = ROOT / 'fixtures' / Path(task_data['repository']).name
        sources = [p for p in fixture.rglob('*') if p.is_file() and p.suffix in ('.h','.cpp','.c','.hpp')]
        row = dict(summary, arm=arm, invoked=bool(positions), invocation_position=first + 1 if first is not None else None,
                   chronological_tool_sequence=chronological,
                   chronological_recovery_distances=recovered,
                   chronological_retries=chronological_retries,
                   trace_order_differs=original_ids != [e['tool_use_id'] for e in events],
                   invoked_skills=[e.get('input', {}).get('skill') for e in events if e.get('tool') == 'Skill'],
                   ast_before_skill=any(e.get('ast_tool') for e in before) if first is not None else None,
                   source_files=len(sources),source_bytes=sum(p.stat().st_size for p in sources),
                   invocation_timestamp=events[first].get('started_at') if first is not None else None,
                   invocation_count=len(positions), first_action=chronological[:1], second_action=chronological[1:2],
                   tools_before_skill=[e['tool'] for e in before] if first is not None else None,
                   exploration_before_skill=any(e['tool'] in ('Read','Grep','Glob','Bash') for e in before) if first is not None else None,
                   failure_before_skill=any(e.get('success') is False for e in before) if first is not None else None,
                   category=task_data.get('evaluation', {}).get('category'), capabilities=task_data.get('evaluation', {}).get('relevant_capabilities', []),
                   expected_files=len(task_data.get('expected_files', [])),
                   prompt_mentions_ast_tool='ast-tool' in task_data['prompt'],
                   prompt_words=len(task_data['prompt'].split()))
        rows.append(row)
    return rows

METRICS = ['success','total_tool_calls','ast_tool_calls','ast_tool_failures','ast_tool_retries','ast_tool_help_calls',
           'grep_calls','glob_calls','read_calls','bash_calls','edit_calls','total_tokens','elapsed_seconds']

def stats(rows):
    if not rows:
        return None
    distances = [d for r in rows for d in r['ast_tool_recovery_distances']]
    return {'runs':len(rows), 'means':{k:mean(float(r[k]) for r in rows) for k in METRICS},
            'medians':{k:median(float(r[k]) for r in rows) for k in METRICS},
            'ranges':{k:[min(r[k] for r in rows),max(r[k] for r in rows)] for k in METRICS},
            'totals':{k:sum(r[k] for r in rows) for k in METRICS},
            'recovery_distances':distances, 'recovery_mean':mean(distances) if distances else None,
            'recovery_max':max(distances) if distances else None}

def wilson(k, n):
    z = 1.96
    p = k / n
    center = (p + z*z/(2*n))/(1+z*z/n)
    radius = z*((p*(1-p)/n+z*z/(4*n*n))**.5)/(1+z*z/n)
    return [max(0.0, center-radius), min(1.0, center+radius)]

def markdown_table(headers, rows):
    def cell(v):
        if v is None:
            return '—'
        if isinstance(v, float):
            return f'{v:.2f}'
        return str(v).replace('|', '\\|')
    return '\n'.join(['| ' + ' | '.join(headers) + ' |', '| ' + ' | '.join(['---']*len(headers)) + ' |'] +
                     ['| ' + ' | '.join(cell(v) for v in row) + ' |' for row in rows])

def export_tables(rows, historical, tables, result):
    lines = ['# Phase 7e measurements', '', 'Generated by `evaluation/phase7e_analyze.py`. These are descriptive measurements, not causal estimates.',
             '', '## Fresh invocation frequency', '',
             markdown_table(['Task','Runs','Invoked','Rate %','Mean position','Class','95% Wilson interval'],
                [[t['task'],t['runs'],t['invoked'],100*t['rate'],t['mean_position'],t['classification'],
                  f"{100*t['wilson_95'][0]:.1f}–{100*t['wilson_95'][1]:.1f}%"] for t in tables]),
             '', '## Fresh run measurements', '',
             markdown_table(['Run','Task','Success','Skills','Position','First','Second','Tools','AST','Fail','Retry','Help','Grep','Glob','Read','Bash','Edit','Tokens','Seconds','Recovery'],
                [[r['arm'],r['task_id'],int(r['success']),', '.join(r['invoked_skills']) or 'none',r['invocation_position'],
                  ', '.join(r['first_action']),', '.join(r['second_action'])] + [r[k] for k in METRICS[1:]] + [str(r['ast_tool_recovery_distances'])] for r in rows]),
             '', '## Same-task arm means', '',
             markdown_table(['Task','Arm','Runs'] + METRICS + ['Recovery mean','Recovery max'],
                [[t['task'],arm,t[arm]['runs']] + [t[arm]['means'][k] for k in METRICS] + [t[arm]['recovery_mean'],t[arm]['recovery_max']]
                 for t in tables for arm in ['loaded','absent'] if t[arm]]),
             '', '## Historical Phase 7d normal task measurements', '',
             markdown_table(['Task','Category','Capabilities','Source files','Source bytes','Prompt words','Skills','Position','First','Tools','Tokens'],
                [[r['task_id'],r['category'],', '.join(r['capabilities']),r['source_files'],r['source_bytes'],r['prompt_words'],
                  ', '.join(r['invoked_skills']) or 'none',r['invocation_position'],', '.join(r['first_action']),r['total_tool_calls'],r['total_tokens']] for r in historical]),
             '', '## Chronological trajectories', '']
    for t in tables:
        lines += [f"### {t['task']}", '']
        for trajectory in t['trajectories']:
            lines += [f"- {trajectory['arm']} ({'semantic loaded' if trajectory['invoked'] else 'semantic absent'}): " + ' → '.join(trajectory['sequence'])]
        lines += ['']
    (OUT / 'tables.md').write_text('\n'.join(lines) + '\n', encoding='utf-8')

def main():
    OUT.mkdir(exist_ok=True)
    historical = collect(ROOT / 'phase7d_full', 'historical-normal')
    phase5 = collect(ROOT.parent / 'experimental', 'phase5-normal', ROOT.parent / 'experimental/traces_phase5', ROOT.parent / 'experimental/results_phase5.jsonl')
    rows = [r for directory in sorted((OUT / 'normal').glob('r[0-9]*')) for r in collect(directory, directory.name)]
    models, listings, versions = set(), set(), set()
    for session in (OUT / 'normal').glob('r*/sessions/*/**/*.jsonl'):
        for line in session.read_text(encoding='utf-8').splitlines():
            event = json.loads(line)
            if event.get('message', {}).get('model'):
                models.add(event['message']['model'])
            if event.get('version'):
                versions.add(event['version'])
            if event.get('attachment', {}).get('type') == 'skill_listing':
                listings.add(event['attachment']['content'])
    skill = ROOT.parent / 'skills/semantic-analysis/SKILL.md'
    old = subprocess.check_output(['git','show','e561505:skills/semantic-analysis/SKILL.md'], cwd=ROOT.parent).decode('utf-8')
    current = skill.read_text(encoding='utf-8')
    added = [line for line in current.splitlines() if line not in old.splitlines()]
    assert len(added) == 1 and added[0].startswith('If a refined `search`')
    assert current.replace(added[0] + '\n', '') == old.replace('\r\n', '\n')
    environment = {'models':sorted(models),'claude_versions':sorted(versions),'skill_listings':sorted(listings),
                   'skill_sha256':hashlib.sha256(skill.read_bytes()).hexdigest(),
                   'phase5_revision':'e561505', 'phase5_sha256':hashlib.sha256(old.encode('utf-8')).hexdigest(), 'exact_added_line':added[0],
                   'binary_sha256':hashlib.sha256(Path('D:/Programs/ast-tool/ast-tool.exe').read_bytes()).hexdigest()}
    environment['harness_sha256'] = hashlib.sha256((ROOT / 'claude.py').read_bytes()).hexdigest()
    environment['auto_memory_files'] = [str(p) for project in (Path.home()/'.claude/projects').glob('*evaluation-repositories*')
                                        for p in (project/'memory').rglob('*') if p.is_file()]
    (OUT / 'environment.json').write_text(json.dumps(environment, indent=2), encoding='utf-8')
    (OUT / 'measurements.json').write_text(json.dumps({'historical':historical,'phase5':phase5,'fresh':rows}, indent=2), encoding='utf-8')
    tables = []
    for task in sorted({r['task_id'] for r in rows}):
        subset = [r for r in rows if r['task_id'] == task]
        loaded = [r for r in subset if r['invoked']]
        absent = [r for r in subset if not r['invoked']]
        rate = len(loaded)/len(subset)
        tables.append({'task':task, 'runs':len(subset),'invoked':len(loaded),'rate':rate,
                       'wilson_95':wilson(len(loaded),len(subset)),
                       'classification':'A' if rate >= .8 else 'B' if rate <= .2 else 'C',
                       'mean_position':mean(r['invocation_position'] for r in loaded) if loaded else None,
                       'loaded':stats(loaded),'absent':stats(absent),
                       'trajectories':[{'arm':r['arm'],'invoked':r['invoked'],'sequence':r['chronological_tool_sequence']} for r in subset]})
    result = {'tasks':tables, 'aggregate':stats(rows),'loaded':stats([r for r in rows if r['invoked']]),
              'absent':stats([r for r in rows if not r['invoked']]),
              'historical_invocation':{str(level):{'runs':len(s := [r for r in historical if r['task_level']==level]),'invoked':sum(r['invoked'] for r in s)} for level in [None,1,2,3,4,5]}}
    mixed = [t for t in tables if t['loaded'] and t['absent']]
    result['same_task_mean_deltas_loaded_minus_absent'] = {
        metric:mean(t['loaded']['means'][metric]-t['absent']['means'][metric] for t in mixed)
        for metric in METRICS} if mixed else None
    result['first_actions'] = dict(Counter(r['first_action'][0] for r in rows if r['first_action']))
    result['skill_types'] = dict(Counter(s for r in rows for s in r['invoked_skills']))
    result['no_skill_at_all'] = stats([r for r in rows if not r['invoked_skills']])
    result['other_skill_only'] = stats([r for r in rows if r['invoked_skills'] and not r['invoked']])
    result['baseline_aggregate'] = stats([r for r in rows if int(r['arm'][1:]) <= 5])
    result['extension_aggregate'] = stats([r for r in rows if int(r['arm'][1:]) > 5])
    result['invocation_positions'] = dict(Counter(r['invocation_position'] for r in rows if r['invoked']))
    result['category_correlations'] = {
        str(category):{'runs':len(s := [r for r in rows if r['category']==category]),'invoked':sum(r['invoked'] for r in s)}
        for category in sorted({r['category'] for r in rows}, key=str)}
    result['capability_correlations'] = {
        capability:{'runs':len(s := [r for r in rows if capability in r['capabilities']]), 'invoked':sum(r['invoked'] for r in s)}
        for capability in sorted({c for r in rows for c in r['capabilities']})}
    phase5_by_task = {r['task_id']:r for r in phase5}
    result['historical_cohorts'] = {}
    for identity in ['invoked', 'invoked_skills']:
        groups = {'both':[], 'neither':[], 'mismatch':[]}
        for row in historical:
            before = bool(phase5_by_task[row['task_id']][identity])
            after = bool(row[identity])
            groups['both' if before and after else 'neither' if not before and not after else 'mismatch'].append(row)
        result['historical_cohorts'][identity] = {
            group:{'tasks':[r['task_id'] for r in subset],
                   'deltas':{k:sum(r[k]-phase5_by_task[r['task_id']][k] for r in subset) for k in METRICS}}
            for group,subset in groups.items()}
    (OUT / 'summary.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    export_tables(rows, historical, tables, result)
    print(json.dumps({'tasks':[{k:v for k,v in t.items() if k not in ('loaded','absent','trajectories')} for t in tables],
                      'historical':result['historical_invocation'], 'aggregate':result['aggregate']}, indent=2))

if __name__ == '__main__':
    main()
