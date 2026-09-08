# Phase 7e evidence

Run unchanged evaluation tasks, then rebuild measurements:

```powershell
evaluation\venv\Scripts\python.exe evaluation\phase7e_probe.py
evaluation\venv\Scripts\python.exe evaluation\phase7e_analyze.py
```

The probe executes eight existing tasks in five sequential rounds, rotating task order. Each task starts from the evaluation fixture repository's committed revision. `AST_TOOL_CONTROLLED_SKILL=1` is rejected. The installed and repository semantic-analysis files must both match the accepted Phase 7d SHA-256 before and after every run. No Skill file or discovery field is modified.

The driver requires the ordinary Claude runtime to access its configuration, session log directory, and service. It preserves existing Claude logs, captures only newly created logs for the evaluated project, and supplies those copies to the unchanged runner's trace and token parsers. Do not run another evaluation against the same fixture repository concurrently.

Additional repetitions can target mixed tasks without changing their prompts:

```powershell
evaluation\venv\Scripts\python.exe evaluation\phase7e_probe.py --first-repeat 6 --last-repeat 10 --task level3-007
```

## Evidence layout

- `normal/manifest.json`: branch revision and per-run Skill hash checks.
- `normal/rN/results.jsonl`: original validator, token, and tool results.
- `normal/rN/traces/`: original per-tool events, inputs, outputs, and timestamps.
- `normal/rN/sessions/`: new evaluation session logs and captured process results; includes child-agent logs where present.
- `normal/rN/analysis/`: existing trace analyzer's metrics.
- `measurements.json`: task-level historical Phase 5, historical Phase 7d, and fresh normal measurements, separated into distinct arrays.
- `summary.json`: invocation frequencies, arm means, aggregate metrics, and equal-task-weighted comparisons where both invocation states occurred.
- `tables.md`: generated, readable raw measurements and chronological trajectories.
- `environment.json`: observed model/version/discovery listings, binary hash, and exact Phase 5-to-7d one-line verification.
- `r1/`: excluded sandbox attempt, which timed out without any tool/session data. This is infrastructure evidence and is not in the normal experiment.

## Measurement definitions

Semantic invocation means a successful `Skill` tool call whose Skill name is `semantic-analysis` (allowing a namespaced prefix). `api-review` and `ast-inspection` remain separate. All tool counts include child-agent activity captured by the existing harness. Invocation position and displayed trajectories are ordered by tool-start timestamp; the original trace sequence is preserved because file modification order can place child tools before their parent `Agent` call. Concurrent starts do not imply a causal order.

Tokens mean the existing harness's input plus output token count; cache fields remain in original results. Tool categories count their named tool, so `Read` excludes shell `cat`, and `Edit` excludes `Write`. AST calls, failures, retries, help, and recovery retain the existing analyzer's definitions. Recovery is the number of tool events from a failed AST call to the next successful non-help AST call; terminal failures have no distance, not distance zero. Recovery means pool recovery events rather than averaging run means. The report checks whether trace reordering affects this metric.

Classes A/B/C use the requested observed thresholds (80–100%, 0–20%, 21–79%). Finite samples cannot establish determinism. Wilson intervals are descriptive binomial intervals; runs share a service and environment and are not proven independent. Loaded-versus-absent arm comparisons are observational, including same-task comparisons. Task, fixture, validator, model selection, and Skill body are held fixed; invocation is not randomized.
