#!/usr/bin/env python3
"""Unit tests for token parsing and aggregation in analyze_results."""
from __future__ import annotations

import unittest

from analyze_results import (
    load_results,
    extract_token_usage,
    summarize_results,
    safe_median,
    get_level,
)
from pathlib import Path
import json
import tempfile


# ---------------------------------------------------------------------------
# extract_token_usage
# ---------------------------------------------------------------------------

class TestExtractTokenUsage(unittest.TestCase):

    def test_normal_token_data(self) -> None:
        result = {
            "tokens": {
                "input": 100,
                "output": 50,
                "cache_read": 0,
                "cache_creation": 0,
            }
        }
        usage = extract_token_usage(result)
        self.assertEqual(usage["input"], 100)
        self.assertEqual(usage["output"], 50)
        self.assertEqual(usage["cache_read"], 0)
        self.assertEqual(usage["cache_creation"], 0)
        self.assertEqual(usage["total"], 150)

    def test_existing_total_not_stored(self) -> None:
        # The schema does not include a pre-computed total field; total is
        # derived from the four component fields.
        result = {
            "tokens": {
                "input": 100,
                "output": 50,
            }
        }
        usage = extract_token_usage(result)
        self.assertEqual(usage["total"], 150)

    def test_missing_token_data(self) -> None:
        result = {"task_id": "level1-001", "success": True}
        usage = extract_token_usage(result)
        self.assertIsNone(usage["input"])
        self.assertIsNone(usage["output"])
        self.assertIsNone(usage["cache_read"])
        self.assertIsNone(usage["cache_creation"])
        self.assertIsNone(usage["total"])

    def test_tokens_field_none(self) -> None:
        result = {"tokens": None}
        usage = extract_token_usage(result)
        self.assertIsNone(usage["total"])

    def test_partial_token_data_no_fabrication(self) -> None:
        result = {"tokens": {"input": 100}}
        usage = extract_token_usage(result)
        self.assertEqual(usage["input"], 100)
        self.assertIsNone(usage["output"])
        # total is computed from available parts only
        self.assertEqual(usage["total"], 100)

    def test_negative_value_treated_as_invalid(self) -> None:
        result = {"tokens": {"input": -100, "output": 50}}
        usage = extract_token_usage(result)
        self.assertIsNone(usage["input"])
        self.assertEqual(usage["output"], 50)
        self.assertEqual(usage["total"], 50)

    def test_non_numeric_value_treated_as_invalid(self) -> None:
        result = {"tokens": {"input": "unknown", "output": 50}}
        usage = extract_token_usage(result)
        self.assertIsNone(usage["input"])
        self.assertEqual(usage["output"], 50)

    def test_null_field_treated_as_missing(self) -> None:
        result = {"tokens": {"input": None, "output": 50}}
        usage = extract_token_usage(result)
        self.assertIsNone(usage["input"])
        self.assertEqual(usage["output"], 50)

    def test_cache_tokens_included_in_total(self) -> None:
        result = {
            "tokens": {
                "input": 10,
                "output": 20,
                "cache_read": 5,
                "cache_creation": 3,
            }
        }
        usage = extract_token_usage(result)
        self.assertEqual(usage["total"], 38)

    def test_all_invalid_gives_none_total(self) -> None:
        result = {"tokens": {"input": -1, "output": "bad"}}
        usage = extract_token_usage(result)
        self.assertIsNone(usage["total"])

    def test_empty_tokens_dict(self) -> None:
        result = {"tokens": {}}
        usage = extract_token_usage(result)
        self.assertIsNone(usage["input"])
        self.assertIsNone(usage["total"])


class TestLoadResults(unittest.TestCase):

    def test_last_rerun_per_task_is_authoritative(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "results.jsonl"
            records = [
                {"task_id": "level1-001", "success": False},
                {"task_id": "level1-002", "success": True},
                {"task_id": "level1-001", "success": True},
            ]
            path.write_text(
                "".join(json.dumps(r) + "\n" for r in records),
                encoding="utf-8",
            )
            loaded = load_results(path)
            self.assertEqual(len(loaded), 2)
            by_id = {r["task_id"]: r for r in loaded}
            self.assertTrue(by_id["level1-001"]["success"])


# ---------------------------------------------------------------------------
# summarize_results — aggregation
# ---------------------------------------------------------------------------

def _make_result(
    task_id: str = "level1-001",
    success: bool = True,
    tokens: dict | None = None,
    ast_tool: dict | None = None,
    tools: dict | None = None,
) -> dict:
    r: dict = {
        "task_id": task_id,
        "success": success,
        "timed_out": False,
        "elapsed_seconds": 10.0,
        "tools": tools or {"Read": 1},
        "ast_tool": ast_tool or {},
    }
    if tokens is not None:
        r["tokens"] = tokens
    return r


class TestAggregationWithMissingData(unittest.TestCase):

    def test_average_excludes_missing(self) -> None:
        results = [
            _make_result("level1-001", tokens={"input": 500, "output": 500}),
            _make_result("level1-002"),
            _make_result("level1-003", tokens={"input": 1500, "output": 1500}),
        ]
        summary = summarize_results(results)
        totals = summary["tokens"]["total"]
        self.assertEqual(len(totals), 2)
        self.assertAlmostEqual(sum(totals) / len(totals), 2000.0)

    def test_tokens_with_data_count(self) -> None:
        results = [
            _make_result("level1-001", tokens={"input": 100, "output": 50}),
            _make_result("level1-002"),
            _make_result("level1-003", tokens={"input": 200, "output": 100}),
        ]
        summary = summarize_results(results)
        self.assertEqual(summary["tokens_with_data"], 2)
        self.assertEqual(summary["total_tasks"], 3)

    def test_analysis_continues_without_tokens(self) -> None:
        results = [_make_result("level1-001")]
        summary = summarize_results(results)
        self.assertEqual(summary["total_tasks"], 1)
        self.assertEqual(summary["tokens_with_data"], 0)
        self.assertEqual(summary["tokens"]["total"], [])


class TestMedianMinMax(unittest.TestCase):

    def test_median(self) -> None:
        totals = [1000, 3000, 2000]
        self.assertEqual(safe_median(totals), 2000)

    def test_aggregated_median(self) -> None:
        results = [
            _make_result("level1-001", tokens={"input": 500, "output": 500}),
            _make_result("level1-002", tokens={"input": 1500, "output": 1500}),
            _make_result("level1-003", tokens={"input": 1000, "output": 1000}),
        ]
        summary = summarize_results(results)
        totals = summary["tokens"]["total"]
        self.assertEqual(sorted(totals), [1000, 2000, 3000])
        self.assertEqual(safe_median(totals), 2000)

    def test_min_max(self) -> None:
        results = [
            _make_result("level1-001", tokens={"input": 500, "output": 500}),
            _make_result("level1-002", tokens={"input": 40000, "output": 10000}),
            _make_result("level1-003", tokens={"input": 1000, "output": 1000}),
        ]
        summary = summarize_results(results)
        totals = summary["tokens"]["total"]
        self.assertEqual(min(totals), 1000)
        self.assertEqual(max(totals), 50000)


class TestLevelGrouping(unittest.TestCase):

    def test_level_token_averages_independent(self) -> None:
        results = [
            _make_result("level1-001", tokens={"input": 100, "output": 100}),
            _make_result("level1-002", tokens={"input": 300, "output": 300}),
            _make_result("level2-001", tokens={"input": 1000, "output": 1000}),
            _make_result("level2-002", tokens={"input": 3000, "output": 3000}),
        ]
        summary = summarize_results(results)

        l1_total = summary["levels"]["level1"]["tokens"]["total"]
        l2_total = summary["levels"]["level2"]["tokens"]["total"]

        self.assertAlmostEqual(sum(l1_total) / len(l1_total), 400.0)
        self.assertAlmostEqual(sum(l2_total) / len(l2_total), 4000.0)

    def test_level_extraction(self) -> None:
        self.assertEqual(get_level("level1-001"), "level1")
        self.assertEqual(get_level("level2-004"), "level2")
        self.assertEqual(get_level("smoke-001"), "smoke")
        self.assertEqual(get_level("nohyphen"), "unknown")


class TestSuccessFailureGrouping(unittest.TestCase):

    def test_success_failure_tokens_separate(self) -> None:
        results = [
            _make_result(
                "level1-001",
                success=True,
                tokens={"input": 100, "output": 100},
            ),
            _make_result(
                "level1-002",
                success=True,
                tokens={"input": 200, "output": 200},
            ),
            _make_result(
                "level1-003",
                success=False,
                tokens={"input": 5000, "output": 5000},
            ),
        ]
        summary = summarize_results(results)

        success_total = summary["success_tokens"]["total"]
        failure_total = summary["failure_tokens"]["total"]

        self.assertEqual(len(success_total), 2)
        self.assertEqual(len(failure_total), 1)
        self.assertAlmostEqual(
            sum(success_total) / len(success_total), 300.0
        )
        self.assertEqual(failure_total[0], 10000)

    def test_failure_missing_tokens_not_counted(self) -> None:
        results = [
            _make_result("level1-001", success=False),
        ]
        summary = summarize_results(results)
        self.assertEqual(summary["failure_tokens"]["total"], [])


class TestAstToolGrouping(unittest.TestCase):

    def test_ast_tool_vs_no_ast_tool(self) -> None:
        results = [
            _make_result(
                "level1-001",
                ast_tool={"search": 2},
                tokens={"input": 200, "output": 200},
            ),
            _make_result(
                "level1-002",
                ast_tool={},
                tokens={"input": 50, "output": 50},
            ),
        ]
        summary = summarize_results(results)

        self.assertEqual(
            summary["ast_tool_used_tokens"]["total"], [400]
        )
        self.assertEqual(
            summary["ast_tool_not_used_tokens"]["total"], [100]
        )
        self.assertEqual(summary["ast_tool_used_count"], 1)
        self.assertEqual(summary["ast_tool_not_used_count"], 1)


class TestInvalidValues(unittest.TestCase):

    def test_negative_does_not_corrupt_aggregate(self) -> None:
        results = [
            _make_result(
                "level1-001",
                tokens={"input": -100, "output": 50},
            ),
            _make_result(
                "level1-002",
                tokens={"input": 200, "output": 100},
            ),
        ]
        summary = summarize_results(results)
        # First result: input invalid, output=50, total=50
        # Second result: input=200, output=100, total=300
        totals = summary["tokens"]["total"]
        self.assertIn(50, totals)
        self.assertIn(300, totals)
        self.assertNotIn(-50, totals)

    def test_string_value_does_not_crash(self) -> None:
        results = [
            _make_result(
                "level1-001",
                tokens={"input": "unknown", "output": 50},
            ),
        ]
        summary = summarize_results(results)
        self.assertEqual(summary["total_tasks"], 1)
        # output=50, total=50
        self.assertEqual(summary["tokens"]["output"], [50])

    def test_null_field_does_not_crash(self) -> None:
        results = [
            _make_result(
                "level1-001",
                tokens={"input": None, "output": 50},
            ),
        ]
        summary = summarize_results(results)
        self.assertEqual(summary["total_tasks"], 1)


if __name__ == "__main__":
    unittest.main()
