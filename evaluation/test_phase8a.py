import json
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
BIN = ROOT / "bin" / "ast-tool.exe"


def run(*arguments):
    return subprocess.run(
        [str(BIN), *arguments], cwd=ROOT, capture_output=True, text=True, encoding="utf-8"
    )


def assert_partial_equals_exact(command, partial, exact, workspace):
    partial_result = run(command, partial, workspace)
    exact_result = run(command, exact, workspace)
    assert partial_result.returncode == exact_result.returncode == 0
    assert partial_result.stdout == exact_result.stdout
    assert partial_result.stdout.strip()


def test_unique_suffix_through_callers():
    assert_partial_equals_exact(
        "callers", "inner::suffixCalTarget", "suffixcal::inner::suffixCalTarget",
        "test/ast-callers/workspace",
    )


def test_unique_suffix_through_callees():
    assert_partial_equals_exact(
        "callees", "inner::suffixCeSource", "suffixce::inner::suffixCeSource",
        "test/ast-callees/workspace",
    )


def test_unique_suffix_through_references():
    assert_partial_equals_exact(
        "references", "inner::suffixRefTarget", "suffixref::inner::suffixRefTarget",
        "test/ast-references/workspace",
    )


def test_phase7f_auth_replay_answer_is_unchanged():
    assert_partial_equals_exact(
        "callers", "AuthToken::expire", "auth::AuthToken::expire",
        "evaluation/repositories/level2-auth",
    )


def test_phase7f_pipeline_replay_answer_is_unchanged():
    assert_partial_equals_exact(
        "callers", "DataStore::save", "store::DataStore::save",
        "evaluation/repositories/level3-pipeline",
    )


def test_missing_partial_fqn_stays_not_found():
    result = run(
        "callers", "MissingToken::expire", "evaluation/repositories/level2-auth"
    )
    assert result.returncode != 0
    assert "symbol not found" in result.stderr


def test_member_receiver_resolution_is_unchanged():
    result = run(
        "callers", "AuthToken::validate", "evaluation/repositories/level2-auth"
    )
    assert result.returncode == 0
    assert not result.stdout.strip()
    assert "no callers found" in result.stderr


def test_controlled_agent_dataset_is_complete_and_recovery_free():
    measurements = json.loads(
        (ROOT / "evaluation/phase8a/measurements.json").read_text(encoding="utf-8")
    )
    runs = measurements["agent_runs"]
    assert len(runs) == 10
    assert all(run["success"] and run["validation_success"] for run in runs)
    assert all(run["tool_sequence"][0] == "Skill" for run in runs)
    assert all(run["ast_tool_failures"] == 0 and run["ast_tool_retries"] == 0 for run in runs)
    assert all(run["trajectory"] != "callers→search→callers" for run in runs)


def test_direct_replays_match_exact_answers():
    measurements = json.loads(
        (ROOT / "evaluation/phase8a/measurements.json").read_text(encoding="utf-8")
    )
    assert len(measurements["direct_replays"]) == 2
    for replay in measurements["direct_replays"]:
        assert replay["answers_equivalent"]
        assert replay["after_ast_calls"] == 1
        assert replay["after_failures"] == 0
