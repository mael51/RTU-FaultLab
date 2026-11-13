"""Generate requirements-to-test traceability matrix."""

import yaml
import json


def build_traceability(requirements_path: str, results: list, path: str):
    with open(requirements_path, "r", encoding="utf-8") as f:
        req = yaml.safe_load(f)
    result_map = {r["id"]: r["passed"] for r in results}
    matrix = {}
    for rid, info in req.get("requirements", {}).items():
        tests = info.get("tests", [])
        matrix[rid] = {
            "description": info.get("description"),
            "tests": tests,
            "results": {t: result_map.get(t) for t in tests},
        }
    with open(path, "w", encoding="utf-8") as f:
        json.dump(matrix, f, indent=2)