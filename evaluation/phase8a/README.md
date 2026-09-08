# Phase 8a raw evidence

The Phase 8a evidence is split from the interpretation in `phase8a-report.md`.

- `measurements.json` contains normalized Phase 7f baselines, ten Phase 8a agent
  runs, per-task aggregates, direct replay timings, and answer sets.
- `agent/r1` through `agent/r5` contain raw results, tool traces, captured fresh
  Claude session logs, and normalized analyses.
- `agent/manifest.json` records the forced semantic condition and Skill/binary
  hashes checked around every run.
- `regression-comparison.json` compares the same test additions against the
  committed resolver and Phase 8a resolver.
- `tables.md` presents the principal raw measurements.

Reproduce the deterministic evidence from the repository root:

```powershell
cmake --build build --config Release --target ast-tool ast-tool-test
python evaluation/phase8a_analyze.py
python evaluation/phase8a_regression_compare.py
python -m pytest evaluation --basetemp=evaluation/.pytest-phase8a
```

`phase8a_probe.py` runs five controlled semantic repetitions of `level2-008`
and `level3-008`. It prepends the workspace `bin` directory to the child agent's
PATH and preserves newly created Claude logs. Running it invokes an external
agent and should only be done when new stochastic evidence is required.

The C++ build needs a normalized Windows environment on this machine because
the inherited native environment can contain case-duplicate `PATH`/`Path`
entries that MSBuild rejects. The regression comparison script performs this
normalization for its child build only.
