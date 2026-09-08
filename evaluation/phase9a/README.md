# Phase 9a — Controlled Semantic Capability Evaluation

## Structure

```
phase9a/
  direct_replays.json     — Direct CLI semantic probes (8 probes, 5 runs each)
  agent/                  — Agent session data (future: Arm A vs Arm B runs)
  README.md               — This file
```

## Direct Probes

Collected 2025-09-07 with Phase 8a+8b+8c binary.

All 8 probes stable across 5 runs. Zero missing. Zero unexpected.

## Agent Sessions

Agent session collection requires running `phase9a_probe.py` with `--arm b` (Phase 8 binary)
and `--arm a` (pre-Phase-8 binary, checkout commit ccc1fbb650bf058aa11602134d4e4fa1795cb98e).

Phase 8a agent data is in `phase8a/agent/` (5 runs × 2 tasks, both Arm A proxy and Arm B).
Phase 8b agent data is in `phase8b/agent/` (5 runs × 2 tasks, Arm B only).
