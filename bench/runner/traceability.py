"""Generate requirements-to-test traceability matrix with strict statuses.

Status semantics (STAB-F03):
- ``COVERED_PASS``      : test executed in this run and passed
- ``COVERED_FAIL``      : test executed in this run and failed
- ``NOT_EXECUTED``      : test defined but not part of this run
- ``NOT_COVERED``       : requirement has no test at all
- ``INVALID_REFERENCE`` : referenced test id is defined nowhere

A MUST requirement is only "covered" when at least one of its tests was
actually executed and passed in the run.
"""

import json

import yaml

COVERED = "COVERED_PASS"
COVERED_FAIL = "COVERED_FAIL"
NOT_EXECUTED = "NOT_EXECUTED"
NOT_COVERED = "NOT_COVERED"
INVALID_REFERENCE = "INVALID_REFERENCE"


def build_traceability(requirements_path: str, results: list, path: str) -> dict:
    with open(requirements_path, "r", encoding="utf-8") as f:
        req = yaml.safe_load(f)

    defined = set(req.get("test_cases", {}).keys())
    executed = {}
    for r in results:
        if r["id"] in executed and executed[r["id"]] != r["passed"]:
            # same id executed twice with a different verdict -> inconsistency
            executed[r["id"]] = "INCONSISTENT"
        else:
            executed[r["id"]] = bool(r["passed"])

    matrix = {}
    for rid, info in req.get("requirements", {}).items():
        tests = info.get("tests", []) or []
        statuses = {}
        for t in tests:
            if t not in defined and t not in executed:
                statuses[t] = INVALID_REFERENCE
            elif t not in executed:
                statuses[t] = NOT_EXECUTED
            elif executed[t] == "INCONSISTENT":
                statuses[t] = COVERED_FAIL
            elif executed[t]:
                statuses[t] = COVERED
            else:
                statuses[t] = COVERED_FAIL

        if not tests:
            overall = NOT_COVERED
        elif any(s == INVALID_REFERENCE for s in statuses.values()):
            overall = INVALID_REFERENCE
        elif any(s == COVERED_FAIL for s in statuses.values()):
            overall = COVERED_FAIL
        elif any(s == COVERED for s in statuses.values()):
            overall = COVERED
        else:
            overall = NOT_EXECUTED

        matrix[rid] = {
            "description": info.get("description"),
            "priority": info.get("priority"),
            "tests": tests,
            "statuses": statuses,
            "overall": overall,
        }

    with open(path, "w", encoding="utf-8") as f:
        json.dump(matrix, f, indent=2)
    return matrix


def traceability_violations(matrix: dict) -> list:
    """Return release-blocking violations (STAB-F03).

    Blocking:
    - a MUST requirement with no test at all (NOT_COVERED);
    - a MUST requirement whose tests were executed and failed (COVERED_FAIL);
    - any requirement referencing an undefined test (INVALID_REFERENCE).

    NOT_EXECUTED is intentionally NOT blocking: those tests may be covered by
    other levels (unit/integration) outside the campaign being analyzed.
    """
    violations = []
    for rid, info in matrix.items():
        overall = info.get("overall")
        if overall == INVALID_REFERENCE:
            violations.append(
                f"{rid} ({info.get('priority')}) references undefined tests: "
                f"{[t for t, s in info.get('statuses', {}).items() if s == INVALID_REFERENCE]}"
            )
        elif info.get("priority") == "MUST" and overall in (NOT_COVERED, COVERED_FAIL):
            violations.append(f"{rid} (MUST) overall={overall} tests={info.get('tests')}")
    return violations
