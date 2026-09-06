import json
from pathlib import Path


DATA = Path(__file__).parent / "phase7f" / "audit.json"


def load_audit():
    return json.loads(DATA.read_text(encoding="utf-8"))


def test_every_ast_call_has_required_annotation_fields():
    audit = load_audit()
    required = {
        "command", "target", "success", "information_gained",
        "reason_for_call", "next_action", "classification",
        "hindsight_assessment",
    }
    assert audit["annotations"]
    assert all(required <= row.keys() for row in audit["annotations"])
    assert all(row["classification"] in "ABCDEF" for row in audit["annotations"])


def test_cohort_has_paired_routing_evidence():
    audit = load_audit()
    assert len(audit["environment"]["cohort"]) == 12
    for task in audit["environment"]["cohort"]:
        comparison = audit["task_comparisons"][task]
        assert comparison["semantic"]["runs"] >= 1
        assert comparison["nonsemantic"]["runs"] >= 1


def test_aggregates_account_for_every_annotation():
    audit = load_audit()
    command_calls = sum(row["calls"] for row in audit["command_summary"].values())
    class_calls = sum(audit["annotation_counts"].values())
    assert command_calls == class_calls == len(audit["annotations"])
    assert len(audit["short_name_recoveries"]) == 4
