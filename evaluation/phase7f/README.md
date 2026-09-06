# Phase 7f raw evidence

Run from the repository root:

```powershell
python evaluation/phase7f_analyze.py
python -m pytest evaluation/test_phase7f_audit.py
```

Files:

- `runs.json` contains the 59 normalized run measurements.
- `ast-call-annotations.json` and `.csv` contain one row per AST call.
- `audit.json` contains the raw runs, annotations, aggregates, and detected patterns.
- `tables.md` presents the principal raw measurements without interpretation.

The analyzer reads the preserved Phase 7d controlled traces, historical no-Skill
traces, and Phase 7e normal traces. It does not run an agent or modify a fixture.
The A-F call labels are deterministic, evidence-guided annotations. They were
reviewed against the original trajectories; they are not model-generated ground
truth.
