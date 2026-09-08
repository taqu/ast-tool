# Phase 8b raw evidence

Interpretation is in `phase8b-report.md` at the repository root. This directory
keeps the raw and normalized measurements separate:

- `measurements.json` contains the Phase 7f baselines, ten final controlled
  agent runs, direct before/after relationship replays, the typed-receiver
  fixture matrix, and existing-relationship replays.
- `agent/r1` through `agent/r5` contain the final binary's raw results, traces,
  fresh Claude session logs, and normalized analyses.
- `agent/manifest.json` records the forced semantic condition and Skill/binary
  hashes checked around every final run.
- `agent-pilot` preserves the first ten runs made before the local-declaration
  and lexical-scope guards were finalized. They are excluded from aggregates.
- `regression-comparison.json` contains three clean-cache monolithic C++ runs
  per source version. The runner's failure membership varies between runs.
- `tables.md` presents the principal measurements.

Reproduce deterministic evidence from the repository root:

```powershell
cmake --build build --config Release --target ast-tool ast-tool-test
python evaluation/phase8b_analyze.py
python evaluation/phase8b_regression_compare.py
python -m pytest evaluation --basetemp=evaluation/.pytest-phase8b
```

`phase8b_probe.py` invokes an external agent five times for each target task.
It forces the accepted semantic Skill, prepends the workspace binary directory
to PATH, and preserves fresh session logs. Run it only when new stochastic
evidence is required.

The C++ build needs a normalized Windows environment on this machine because
the inherited native environment can contain duplicate `PATH`/`Path` entries.
The regression script normalizes them for child builds.
