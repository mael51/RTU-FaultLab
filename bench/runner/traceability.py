"""Generate requirements-to-test traceability matrix with strict statuses."""

import yaml
import json


def _status(result_map: dict, test_id: str) -> str:
    if test_id not in result_map:
        # check if test exists in any campaign? We only know results
        return "NOT_EXECUTED"
    val = result_map[test_id]
    if val is True:
        return "COVERED_PASS"
    if val is False:
        return "COVERED_FAIL"
    return "NOT_EXECUTED"


def build_traceability(requirements_path: str, results: list, path: str,
                      known_tests: set = None):
    with open(requirements_path, "r", encoding="utf-8") as f:
        req = yaml.safe_load(f)
    result_map = {r["id"]: r["passed"] for r in results}
    known = known_tests or set(r["id"] for r in results)

    matrix = {}
    for rid, info in req.get("requirements", {}).items():
        tests = info.get("tests", []) or []
        statuses = {}
        for t in tests:
            if t not in known and t not in result_map:
                statuses[t] = "INVALID_REFERENCE"
            else:
                statuses[t] = _status(result_map, t)
        # determine overall
        if not tests:
            overall = "NOT_COVERED"
        elif any(s == "INVALID_REFERENCE" for s in statuses.values()):
            overall = "INVALID_REFERENCE"
        elif any(s == "COVERED_FAIL" for s in statuses.values()):
            overall = "COVERED_FAIL"
        elif any(s == "COVERED_PASS" for s in statuses.values()):
            overall = "COVERED_PASS"
        else:
            overall = "NOT_EXECUTED"
        matrix[rid] = {
            "description": info.get("description"),
            "priority": info.get("priority"),
            "tests": tests,
            "statuses": statuses,
            "overall": overall,
        }
    with open(path, "w", encoding="utf-8") as f:
        json.dump(matrix, f, indent=2)