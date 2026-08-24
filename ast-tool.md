# Add Resume and Retry-Failed Support to `run_eval.py`

## Goal

Improve `run_eval.py` so that an interrupted or partially completed evaluation does not need to rerun every task.

The runner should inspect the existing `results.jsonl` file and decide which tasks still need to be executed.

The primary desired behavior is:

```text
Task has no result in results.jsonl
    → Run

Task exists and latest result is success: false
    → Run again

Task exists and latest result is success: true
    → Skip
```

This should allow the evaluation to resume automatically after:

```text
Interrupted evaluation
Timeouts
Failed tasks
Agent crashes
Validation failures
Partial evaluation runs
```

---

# 1. Default Resume Behavior

When `run_eval.py` is executed, it should load the existing `results.jsonl` if it exists.

For each evaluation task:

```text
results.jsonl
        ↓
Find previous result for task
        ↓
┌───────────────────────────────┐
│ No previous result            │
│        → RUN                  │
├───────────────────────────────┤
│ Latest result success = false │
│        → RUN                  │
├───────────────────────────────┤
│ Latest result success = true  │
│        → SKIP                 │
└───────────────────────────────┘
```

The goal is that rerunning the same command naturally resumes unfinished work.

For example:

```bash
python run_eval.py
```

If 100 tasks exist and:

```text
80 succeeded
10 failed
10 were never executed
```

the next execution should run:

```text
10 failed
+
10 missing
=
20 tasks
```

The 80 successful tasks should be skipped.

---

# 2. Use the Latest Result Per Task

`results.jsonl` may contain multiple entries for the same task because failed tasks can be retried.

For example:

```json
{"task_id": "level2-001", "success": false}
{"task_id": "level2-001", "success": true}
```

The runner must use the **latest result** for each task.

Conceptually:

```python
latest_results = {}

for result in results_jsonl:
    latest_results[result["task_id"]] = result
```

Then:

```python
previous = latest_results.get(task.id)

if previous is None:
    run_task(task)

elif previous["success"] is True:
    skip_task(task)

else:
    run_task(task)
```

Do not use the first result found.

The last entry in `results.jsonl` should determine the current state of the task.

---

# 3. Preserve Existing Results

Do not rewrite or delete existing successful results.

New evaluation results should continue to be appended to:

```text
results.jsonl
```

For example:

Initial file:

```text
task-001 → success
task-002 → failure
task-003 → success
```

After rerunning:

```text
task-001 → success
task-002 → failure
task-003 → success
task-002 → success
```

The latest result becomes authoritative.

This preserves evaluation history and makes retries observable.

---

# 4. Add an Explicit Resume Mode

If the current behavior always runs all tasks, add an explicit command-line option.

For example:

```bash
python run_eval.py --resume
```

Behavior:

```text
Missing result
    → Run

Previous failure
    → Run

Previous success
    → Skip
```

If this is implemented as the default behavior, `--resume` may still be provided as an explicit alias for clarity.

Recommended options:

```bash
python run_eval.py
```

Run according to the default behavior.

```bash
python run_eval.py --resume
```

Explicitly resume incomplete evaluation.

```bash
python run_eval.py --force
```

Run all selected tasks regardless of previous results.

The exact CLI naming can follow the existing style of the project.

---

# 5. Add `--retry-failed`

Support a mode that runs only previously failed tasks.

For example:

```bash
python run_eval.py --retry-failed
```

Behavior:

```text
Previous result missing
    → Skip

Previous result success = true
    → Skip

Previous result success = false
    → Run
```

This is useful when the evaluation has already completed and only failed tasks should be retried.

---

# 6. Recommended Execution Modes

The desired modes are:

| Mode                | Missing | Failed | Successful |
| ------------------- | ------: | -----: | ---------: |
| Normal / `--resume` |     Run |    Run |       Skip |
| `--retry-failed`    |    Skip |    Run |       Skip |
| `--force`           |     Run |    Run |        Run |

Keep the behavior simple and predictable.

If multiple mode flags are mutually exclusive, argparse should reject invalid combinations.

For example:

```bash
python run_eval.py --retry-failed --force
```

should either be rejected or clearly documented.

Prefer a mutually exclusive argument group.

---

# 7. Agent-Aware Results

The evaluation framework is being extended to support multiple coding agents.

Therefore, task completion state should not be determined by `task_id` alone.

A result should be associated with both:

```text
agent
+
task_id
```

For example:

```json
{
  "agent": "claude",
  "task_id": "level2-001",
  "success": true
}
```

and:

```json
{
  "agent": "antigravity",
  "task_id": "level2-001",
  "success": false
}
```

These must be treated independently.

Conceptually:

```python
latest_results[(agent_name, task_id)] = result
```

Then:

```text
Claude + level2-001
    success
        → skip Claude execution

Antigravity + level2-001
    failure
        → rerun Antigravity execution
```

Do not allow results from one agent to cause another agent's task to be skipped.

---

# 8. Backward Compatibility

Existing result files may not contain an `agent` field.

For backward compatibility, treat old results as belonging to the existing default agent.

For example:

```python
agent = result.get("agent", "claude")
```

Use the actual canonical agent identifier used by the evaluation framework.

Do not require users to regenerate all existing evaluation results.

---

# 9. Missing or Corrupt Results

The runner should handle an empty or missing `results.jsonl`.

Behavior:

```text
results.jsonl does not exist
    → Run all selected tasks

results.jsonl is empty
    → Run all selected tasks
```

If an individual JSONL line is malformed:

```text
Malformed line
    → Warn
    → Skip that line
    → Continue loading other valid results
```

Do not cause the entire evaluation to fail because one historical result line is corrupted.

However, malformed input should be visible to the user through a warning.

---

# 10. Result Loading Helper

Extract result loading into a dedicated helper.

For example:

```python
def load_latest_results(results_path):
    """
    Load results.jsonl and return the latest result
    for each (agent, task_id) pair.
    """
```

Conceptually:

```python
def load_latest_results(results_path):
    latest = {}

    if not results_path.exists():
        return latest

    with results_path.open() as f:
        for line_number, line in enumerate(f, start=1):
            line = line.strip()

            if not line:
                continue

            try:
                result = json.loads(line)
            except json.JSONDecodeError:
                print(
                    f"Warning: invalid JSON "
                    f"in {results_path}:{line_number}"
                )
                continue

            task_id = result.get("task_id")

            if not task_id:
                continue

            agent = result.get("agent", DEFAULT_AGENT)

            latest[(agent, task_id)] = result

    return latest
```

Adapt this to the existing project style.

Avoid duplicating JSONL parsing logic.

---

# 11. Task Selection Helper

Keep the execution decision separate from the actual task execution.

For example:

```python
def should_run_task(
    task,
    agent,
    latest_results,
    mode,
):
    ...
```

Possible behavior:

```python
previous = latest_results.get((agent, task.id))

if mode == "force":
    return True

if mode == "retry_failed":
    return previous is not None and not previous["success"]

if previous is None:
    return True

return not previous["success"]
```

Do not scatter resume logic throughout the main evaluation loop.

The main loop should remain easy to understand.

Ideally:

```python
latest_results = load_latest_results(results_path)

for task in tasks:
    if not should_run_task(
        task,
        agent,
        latest_results,
        mode,
    ):
        print(f"SKIP {task.id}")
        continue

    result = run_task(...)
    append_result(result)
```

---

# 12. Output Summary

Before running tasks, print a selection summary.

For example:

```text
Evaluation selection

Agent: claude
Mode: resume

Total tasks:     100
Already passed:   80
Failed / retry:   10
Missing result:   10
Selected to run:  20
Skipped:          80
```

For `--retry-failed`:

```text
Evaluation selection

Agent: antigravity
Mode: retry-failed

Total tasks:      100
Previously failed: 12
Selected to run:   12
Skipped:           88
```

This makes the runner behavior easy to verify.

---

# 13. Important Edge Case: Interrupted Runs

Consider this sequence:

```text
Task 001 → success
Task 002 → success
Task 003 → currently running
Process interrupted
```

If Task 003 never wrote a result:

```text
Task 003
    → Missing
    → Run on next invocation
```

If Task 003 wrote:

```json
{
  "task_id": "task-003",
  "success": false,
  "failure_type": "interrupted"
}
```

Then:

```text
Task 003
    → Failed
    → Run on next resume
```

Both cases should naturally work with the resume logic.

---

# 14. Important Edge Case: Task Definitions Change

Do not attempt to solve full task versioning in this change.

However, structure the code so that future support for task fingerprints or revisions would be possible.

For now, completion is determined by:

```text
agent
+
task_id
+
latest result success
```

Do not introduce task hashing or schema redesign unless it is trivial and clearly useful.

---

# 15. Tests

Add tests for the selection logic.

At minimum:

### No results

```text
results.jsonl missing
```

Expected:

```text
All tasks run
```

---

### Successful task

```json
{"task_id": "task-001", "success": true}
```

Expected:

```text
task-001 skipped in resume mode
```

---

### Failed task

```json
{"task_id": "task-001", "success": false}
```

Expected:

```text
task-001 runs in resume mode
```

---

### Multiple results

```json
{"task_id": "task-001", "success": false}
{"task_id": "task-001", "success": true}
```

Expected:

```text
task-001 skipped
```

The latest result must win.

---

### Agent separation

```json
{"agent": "claude", "task_id": "task-001", "success": true}
{"agent": "antigravity", "task_id": "task-001", "success": false}
```

Expected:

```text
Claude:
    task-001 skipped

Antigravity:
    task-001 runs
```

---

### Retry failed

Results:

```text
task-001 → success
task-002 → failure
task-003 → missing
```

Command:

```bash
python run_eval.py --retry-failed
```

Expected:

```text
task-001 → skip
task-002 → run
task-003 → skip
```

---

### Force

Results:

```text
task-001 → success
task-002 → failure
task-003 → missing
```

Command:

```bash
python run_eval.py --force
```

Expected:

```text
All tasks run
```

---

### Malformed JSONL

A malformed line should produce a warning but should not stop evaluation.

---

# 16. Acceptance Criteria

The implementation is complete when:

* [ ] `run_eval.py` can inspect an existing `results.jsonl`.
* [ ] Tasks with no previous result can be selected for execution.
* [ ] Previously failed tasks can be selected for execution.
* [ ] Previously successful tasks can be skipped.
* [ ] The latest result for a task is authoritative.
* [ ] Results are tracked independently per agent.
* [ ] Existing results without an `agent` field remain usable.
* [ ] New results continue to append to `results.jsonl`.
* [ ] Interrupted evaluations can be resumed without rerunning successful tasks.
* [ ] `--retry-failed` runs only failed tasks.
* [ ] `--force` runs all tasks.
* [ ] A clear task selection summary is printed.
* [ ] Malformed historical JSONL lines do not crash the evaluation.
* [ ] Selection logic is covered by tests.
* [ ] Existing evaluation behavior is not broken.

---

# Final Principle

The evaluation runner should become safely restartable.

The normal workflow should be:

```bash
python run_eval.py
```

Then, if execution stops for any reason:

```bash
python run_eval.py
```

should continue from the remaining work:

```text
Successful tasks
    → Skip

Failed tasks
    → Retry

Tasks without results
    → Run
```

The implementation should remain simple.

The core idea is:

```text
results.jsonl
        ↓
Latest result per (agent, task)
        ↓
Decide:
    success → skip
    failure → run
    missing → run
```
