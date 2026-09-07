# Phase 11 — Release Freeze / Baseline Lock

## Objective

Phase 11 transitions the project from capability development into release preparation.

The purpose of this phase is to:

```text
freeze the accepted implementation baseline
define the release scope
separate release blockers from known limitations
prevent further feature drift before release qualification
```

This is **not** a semantic capability improvement phase.

Do not introduce new semantic behavior, new commands, new routing strategies, or speculative optimizations unless they are required to resolve a confirmed release blocker.

The primary outcome of Phase 11 is a clearly defined and reproducible release baseline.

---

# Release Baseline

The release baseline is:

```text
Phase 7d Skill
+
Phase 8a
    unique FQN suffix relationship resolution
+
Phase 8b
    receiver-type member relationship resolution
+
Phase 8c
    callable declaration/definition body identity
```

Phase 10 routing experiments are **not** part of the release baseline.

In particular:

```text
Phase 10 Candidate 1
    trigger vocabulary addition
    → REJECTED
    → REVERTED
```

Do not reintroduce rejected Phase 10 changes.

---

# Phase 10 Final Status

Record Phase 10 as:

```text
Phase 10
Semantic Routing Opportunity / Trigger Reliability

Status:
COMPLETE

Final result:
NO SAFE ROUTING IMPROVEMENT FOUND
```

The baseline routing behavior should therefore remain unchanged for the release.

The key conclusion is:

```text
Semantic routing is already reliable for the task shapes
where semantic capability has demonstrated the strongest value.

Remaining missed opportunities are sparse boundary cases,
and no safe routing improvement was demonstrated.
```

Do not continue routing experimentation during release preparation.

---

# Feature Freeze

After Phase 11 begins, apply a feature freeze.

Do not add:

```text
new AST commands
new semantic commands
new relationship semantics
new receiver inference
new resolution strategies
new routing heuristics
new Skill trigger strategies
new agent-specific optimization behavior
new output modes
```

unless a release-blocking defect requires a narrowly scoped change.

The default decision for non-critical enhancement ideas is:

```text
DEFER TO POST-RELEASE
```

---

# Release Scope

Define exactly what the first release promises.

The release should include only capabilities that are already implemented, tested, and understood.

Document the supported command surface and semantic behavior.

At minimum, identify:

```text
supported commands
supported languages
supported relationship queries
supported symbol resolution behavior
supported JSON / machine-readable output
supported repository/workspace behavior
supported operating systems
```

Do not expand support claims based on theoretical capability.

Only document behavior demonstrated by the current implementation and tests.

---

# Known Limitations

Create a release-facing known limitations list.

Known limitations are not automatically release blockers.

Current examples include:

```text
advanced receiver/type inference

auto / decltype-based receiver inference

complex receiver expressions

template-heavy resolution

inheritance-aware relationship resolution

virtual dispatch

overload-sensitive relationship resolution

explicit this-> receiver handling

callee-side residual declaration/definition identity gaps

ambiguous or unsupported language-specific semantic cases
```

Verify the exact current behavior before publishing each limitation.

Do not attempt to fix these merely because they are known.

A limitation should block release only if it violates the release's documented contract or causes unacceptable correctness or stability problems in ordinary supported use.

---

# Rejected and Deferred Work

Create a concise record of important rejected or deferred work.

This should include at least:

```text
Phase 6
    Agent-facing command surface
    REJECTED

Phase 10 Candidate 1
    Trigger vocabulary addition
    REJECTED

Stable semantic symbol ID
    DEFERRED

Additional semantic capability extensions
    DEFERRED

Further semantic routing optimization
    DEFERRED
```

The purpose is to prevent future release work from accidentally reopening already-settled experiments.

Do not delete historical evaluation data.

---

# Release Blocker Definition

A release blocker is a defect that makes the current supported release unsafe, unreliable, unusable, or materially misleading.

Examples include:

```text
crash or panic during normal supported use

repository corruption

incorrect file modification

incorrect exit status

malformed or unstable machine-readable output

installation failure

startup failure

critical cross-platform failure

common valid input producing clearly incorrect semantic results

nondeterministic behavior that violates the documented CLI contract

missing required release files

license or distribution problems
```

Do not classify every known semantic limitation as a release blocker.

---

# Non-Blocker Definition

Examples of issues that normally should not block the release:

```text
unsupported advanced C++ semantics

edge-case receiver forms

unsupported templates or virtual dispatch

minor efficiency differences

marginal semantic-routing false negatives

extra Read/Grep calls when correctness remains intact

feature requests

new commands

additional output convenience

rare unsupported source constructs
```

Record these as limitations or backlog items instead.

---

# Release Baseline Verification

Before declaring the baseline frozen, verify that the repository is in the intended state.

Perform:

```text
1. build from a clean checkout

2. run the complete existing test suite

3. verify no rejected Phase 10 changes remain

4. verify Phase 8a behavior remains present

5. verify Phase 8b behavior remains present

6. verify Phase 8c behavior remains present

7. verify the accepted Phase 7d Skill is present

8. verify no accidental experimental files affect runtime behavior

9. verify working tree cleanliness after tests

10. record the exact commit used as the frozen baseline
```

Do not modify semantics merely to make this verification cleaner.

---

# Regression Gate

Run the normal regression suite against the frozen candidate.

The purpose is not to optimize metrics.

The purpose is to detect accidental regression.

Record at least:

```text
total tests
passed
failed
success rate

AST failures
AST retries

unexpected crashes
unexpected output changes
```

Where existing agent-level evaluation can be run cheaply, use it as a smoke/regression check.

Do not require historical token or latency metrics to improve.

Release readiness is based on stability, not optimization.

---

# CLI Contract Inventory

Create an inventory of the externally visible CLI contract.

For every public command, record:

```text
command name
arguments
required arguments
optional arguments
exit behavior
stdout behavior
stderr behavior
JSON behavior
common failure modes
```

Identify any behavior that is currently unstable or undocumented.

Do not redesign the CLI in Phase 11 unless there is a clear release-blocking problem.

CLI polish belongs to the later release-preparation phase.

---

# Versioning Decision

Choose the intended initial release version.

Prefer a pre-1.0 version unless the project already has a stronger compatibility commitment.

For example:

```text
v0.1.0
```

or an appropriate existing project version.

Record:

```text
release version
version source of truth
how the CLI reports the version
tag format
release branch policy
```

Do not implement a complex versioning system if the project does not need one.

---

# Branch and Change Policy

Define the release preparation policy.

Recommended model:

```text
main / development branch
    normal future development after release branch is cut

release branch
    release-blocking fixes only
```

If the project does not need a separate release branch, document that explicitly.

After freeze, every code change intended for the release should satisfy:

```text
Is this required for release correctness,
stability, compatibility, packaging, documentation,
or distribution?
```

If the answer is no:

```text
defer it
```

---

# Artifact Inventory

Identify all files required for release.

At minimum check for:

```text
README
LICENSE
CHANGELOG or release notes source
installation instructions
usage documentation
supported-platform information
known limitations
version information
CI configuration
release workflow if applicable
third-party license notices if required
```

Phase 11 only inventories these items.

Missing documentation or packaging can be completed in later release phases unless it blocks establishing the baseline.

---

# Issue Classification

Review known open issues and classify each as one of:

```text
RELEASE BLOCKER
RELEASE FIX
KNOWN LIMITATION
POST-RELEASE IMPROVEMENT
REJECTED / NOT PLANNED
```

For every release blocker, record:

```text
issue
impact
reproduction
affected platform / command
required fix
verification method
```

Avoid vague blocker labels.

---

# No Opportunistic Cleanup

Do not perform unrelated cleanup during Phase 11.

Avoid:

```text
large refactors

naming cleanup

architecture cleanup

dependency upgrades without release need

format-only repository-wide changes

test rewrites

performance tuning

new abstractions

semantic simplification
```

These changes increase release risk without helping establish the frozen baseline.

---

# Expected Deliverables

Produce the following.

## 1. Frozen Baseline Record

Include:

```text
commit hash
branch
version target
Phase 7d Skill status
Phase 8a status
Phase 8b status
Phase 8c status
Phase 10 changes excluded
working tree state
```

---

## 2. Release Scope

Document:

```text
supported languages
supported commands
supported semantic capabilities
supported platforms
supported output modes
```

Only claim verified support.

---

## 3. Known Limitations

Create a concise list suitable for later inclusion in release documentation.

Separate:

```text
intentional unsupported behavior
known edge cases
known low-priority defects
```

---

## 4. Release Blocker List

Produce a table such as:

```text
Issue | Severity | Blocker? | Required before release? | Verification
```

If no blockers are found, explicitly state:

```text
No confirmed release blockers found during Phase 11.
```

---

## 5. Deferred / Rejected Work Record

Include major historical decisions so that release preparation does not reopen them.

---

## 6. Release Readiness Summary

End with one of:

```text
BASELINE LOCKED — READY FOR RELEASE QUALIFICATION

BASELINE LOCKED WITH RELEASE BLOCKERS

BASELINE NOT SAFE TO FREEZE
```

If blockers exist, do not broaden the scope.

Resolve only those blockers in the following release-quality phase.

---

# Acceptance Criteria

Phase 11 is complete when:

```text
1. the exact release baseline is identified

2. rejected Phase 10 changes are absent

3. all existing tests pass at the expected baseline level,
   or every failure is understood and classified

4. release scope is documented

5. known limitations are separated from release blockers

6. public CLI surface is inventoried

7. versioning / branch policy is defined

8. no unresolved uncertainty exists about which implementation
   is intended for release

9. feature freeze is in effect
```

Phase 11 does not require the product to be fully packaged or documented.

Those belong to subsequent release-preparation phases.

---

# Final Principle

Phase 11 should answer one question:

```text
What exact version of the project are we preparing to release?
```

At the end of this phase, that answer must be unambiguous.

From this point onward:

```text
stability > optimization

release correctness > new capability

small verified fixes > speculative improvement
```
