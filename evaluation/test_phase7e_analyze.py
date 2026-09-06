import json

import phase7e_analyze as analysis


def test_semantic_identity_and_chronology(tmp_path, monkeypatch):
    monkeypatch.setattr(analysis, 'ROOT', tmp_path)
    (tmp_path / 'tasks').mkdir()
    (tmp_path / 'tasks/level1-001.yaml').write_text(
        'id: level1-001\nrepository: repositories/example\nprompt: Find a symbol\n',
        encoding='utf-8')
    traces = tmp_path / 'traces'
    traces.mkdir()
    events = [
        {'event':'task_start','task_id':'level1-001'},
        {'event':'tool_call','sequence':1,'tool':'Skill','tool_use_id':'child',
         'input':{'skill':'semantic-analysis'},'success':True,'started_at':'2026-01-01T00:00:02Z'},
        {'event':'tool_call','sequence':2,'tool':'Agent','tool_use_id':'parent',
         'input':{},'success':True,'started_at':'2026-01-01T00:00:01Z'},
        {'event':'task_end','success':True},
    ]
    path = traces / 'level1-001.jsonl'
    path.write_text('\n'.join(json.dumps(e) for e in events), encoding='utf-8')
    row = analysis.collect(tmp_path, 'test')[0]
    assert row['invoked'] is True
    assert row['invocation_position'] == 2
    assert row['first_action'] == ['Agent']
    assert row['tools_before_skill'] == ['Agent']
    assert row['trace_order_differs'] is True
    events[1]['input']['skill'] = 'api-review'
    path.write_text('\n'.join(json.dumps(e) for e in events), encoding='utf-8')
    row = analysis.collect(tmp_path, 'test')[0]
    assert row['invoked'] is False
    assert row['invocation_position'] is None
    assert row['invoked_skills'] == ['api-review']
    events[1]['input']['skill'] = 'semantic-analysis'
    events[1]['success'] = False
    path.write_text('\n'.join(json.dumps(e) for e in events), encoding='utf-8')
    assert analysis.collect(tmp_path, 'test')[0]['invoked'] is False
    for event in events[1:3]:
        event['tool'] = 'Bash'
        event['ast_tool'] = {'detected':True, 'command':'search', 'raw_command':'ast-tool search --name f .'}
    events[1]['success'] = True
    events[2]['success'] = False
    path.write_text('\n'.join(json.dumps(e) for e in events), encoding='utf-8')
    row = analysis.collect(tmp_path, 'test')[0]
    assert row['ast_tool_recovery_distances'] == []
    assert row['chronological_recovery_distances'] == [1]
    assert row['ast_tool_retries'] == 0
    assert row['chronological_retries'] == 1


def test_recovery_mean_is_event_weighted():
    rows = [{k: 1 for k in analysis.METRICS} for _ in range(2)]
    rows[0]['ast_tool_recovery_distances'] = [1, 1]
    rows[1]['ast_tool_recovery_distances'] = [4]
    result = analysis.stats(rows)
    assert result['recovery_mean'] == 2
    assert result['recovery_max'] == 4
    assert result['totals']['total_tokens'] == 2
    assert analysis.stats([]) is None
