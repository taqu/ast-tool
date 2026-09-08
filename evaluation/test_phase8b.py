import hashlib
import json
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
BIN = ROOT / "bin" / "ast-tool.exe"
WORKSPACE = "test/ast-member-receiver/workspace"


def run(command, target):
    return subprocess.run(
        [str(BIN), command, target, WORKSPACE],
        cwd=ROOT, capture_output=True, text=True, encoding="utf-8",
    )


def output_lines(command, target):
    result = run(command, target)
    assert result.returncode == 0, result.stderr
    return [line for line in result.stdout.splitlines() if line.strip()]


def test_field_local_reference_and_pointer_callers_are_receiver_constrained():
    lines = output_lines("callers", "typed::Validator::validate")
    assert any("typed::Session::fieldObject" in line for line in lines)
    assert any("typed::Session::fieldPointer" in line for line in lines)
    assert any("typed::localObject" in line for line in lines)
    assert any("typed::referenceParameter" in line for line in lines)
    assert any("typed::pointerParameter" in line for line in lines)
    assert not any("unrelatedField" in line for line in lines)
    assert not any("unresolvedExpression" in line for line in lines)


def test_same_member_name_on_other_field_stays_separate():
    lines = output_lines("callers", "typed::OtherValidator::validate")
    assert len(lines) == 1
    assert "typed::Session::unrelatedField" in lines[0]


def test_field_member_references_are_included_without_false_positives():
    lines = output_lines("references", "typed::Validator::validate")
    assert len(lines) == 5
    assert not any("unrelatedField" in line for line in lines)


def test_field_member_appears_in_callees():
    lines = output_lines("callees", "typed::Session::fieldObject")
    assert len(lines) == 1
    assert lines[0].startswith("typed::Validator::validate ")


def test_each_supported_receiver_appears_in_callees():
    callers = (
        "typed::Session::fieldObject",
        "typed::Session::fieldPointer",
        "typed::localObject",
        "typed::referenceParameter",
        "typed::pointerParameter",
    )
    for caller in callers:
        lines = output_lines("callees", caller)
        assert any(line.startswith("typed::Validator::validate ") for line in lines), caller


def test_local_and_parameter_receivers_resolve_from_direct_declarations():
    callers = output_lines("callers", "typed::Validator::validate")
    supported = ("localObject", "referenceParameter", "pointerParameter")
    assert all(any(name in line for line in callers) for name in supported)


def test_unresolved_receiver_expression_does_not_guess():
    lines = output_lines("callees", "typed::unresolvedExpression")
    assert not any("Validator::validate" in line for line in lines)


def test_overloaded_member_stays_unresolved():
    lines = output_lines("callers", "typed::Overloaded::validate")
    assert not any("ambiguousMember" in line for line in lines)


def test_qualified_receiver_types_in_different_namespaces_stay_separate():
    left = output_lines("callers", "left::Validator::validate")
    right = output_lines("callers", "right::Validator::validate")
    assert len(left) == len(right) == 1
    assert "NamespaceSession::callLeft" in left[0]
    assert "NamespaceSession::callRight" in right[0]


def test_local_shadows_same_named_field_without_type_contamination():
    field_type = output_lines("callers", "shadow_guard::Validator::validate")
    local_type = output_lines("callers", "shadow_guard::OtherValidator::validate")
    assert len(field_type) == len(local_type) == 1
    assert "shadow_guard::Holder::fieldCall" in field_type[0]
    assert "shadow_guard::localWithFieldName" in local_type[0]


def test_out_of_scope_sibling_declaration_is_not_used():
    first_type = output_lines("callers", "scope_guard::Validator::validate")
    visible_type = output_lines("callers", "scope_guard::OtherValidator::validate")
    assert first_type == []
    assert len(visible_type) == 1
    assert "scope_guard::siblingBlocks" in visible_type[0]


def test_controlled_agent_dataset_is_complete_and_skill_controlled():
    measurements = json.loads(
        (ROOT / "evaluation/phase8b/measurements.json").read_text(encoding="utf-8")
    )
    runs = measurements["agent_runs"]
    assert len(runs) == 10
    assert {run["task_id"] for run in runs} == {"level2-004", "level4-006"}
    assert all(run["skill_first"] for run in runs)
    assert all(run["ast_tool_failures"] == 0 and run["ast_tool_retries"] == 0
               for run in runs)
    assert sum(run["success"] for run in runs if run["task_id"] == "level2-004") == 5
    assert measurements["level4_validator_defect"]["intended_three_edits_observed_in_sessions"] == 5


def test_agent_manifest_preserves_skill_and_evaluated_binary():
    measurements = json.loads(
        (ROOT / "evaluation/phase8b/measurements.json").read_text(encoding="utf-8")
    )
    environment = measurements["environment"]
    assert environment["skill_sha256"] == environment["installed_skill_sha256"]
    assert len(measurements["agent_manifest_checks"]) == 10
    assert all(check["binary_after"] == environment["agent_binary_sha256"]
               for check in measurements["agent_manifest_checks"])
    assert hashlib.sha256((ROOT / "skills/semantic-analysis/SKILL.md").read_bytes()).hexdigest() \
        == environment["skill_sha256"]


def test_recorded_relationship_replays_are_exact_and_stable():
    measurements = json.loads(
        (ROOT / "evaluation/phase8b/measurements.json").read_text(encoding="utf-8")
    )
    assert len(measurements["direct_replays"]) == 4
    for replay in measurements["direct_replays"]:
        assert replay["stable_across_five"]
        assert replay["missing"] == []
        assert replay["unexpected"] == []
        assert all(run["returncode"] == 0 for run in replay["runs"])


def test_recorded_fixture_matrix_has_no_missing_or_false_relationships():
    measurements = json.loads(
        (ROOT / "evaluation/phase8b/measurements.json").read_text(encoding="utf-8")
    )
    fixtures = measurements["fixture_measurements"]
    assert len(fixtures) == 16
    assert all(row["returncode"] == 0 for row in fixtures)
    assert all(row["missing"] == [] and row["unexpected"] == [] for row in fixtures)


def test_existing_relationship_replays_stay_stable():
    measurements = json.loads(
        (ROOT / "evaluation/phase8b/measurements.json").read_text(encoding="utf-8")
    )
    rows = measurements["existing_relationship_replays"]
    assert len(rows) == 6
    assert all(row["stable_across_five"] for row in rows)
    assert all(row["missing"] == [] and row["unexpected_count"] == 0 for row in rows)
