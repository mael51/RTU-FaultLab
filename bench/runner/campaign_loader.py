"""Load and validate a campaign YAML file."""

import yaml
from dataclasses import dataclass, field
from typing import Any, Dict, List


@dataclass
class TestCase:
    id: str
    requirement: Any
    action: str
    address: int
    count: int = 1
    value: int = 0
    inject: Dict[str, Any] = field(default_factory=dict)
    expect: Dict[str, Any] = field(default_factory=dict)


@dataclass
class Campaign:
    id: str
    seed: int
    target: str
    timeout_ms: int
    tests: List[TestCase]


REQUIRED_TOP = ["campaign"]
REQUIRED_CAMP = ["id", "seed", "target", "timeout_ms", "tests"]


def load_campaign(path: str) -> Campaign:
    with open(path, "r", encoding="utf-8") as f:
        raw = yaml.safe_load(f)
    if not isinstance(raw, dict) or "campaign" not in raw:
        raise ValueError("missing 'campaign' top-level key")
    c = raw["campaign"]
    for key in REQUIRED_CAMP:
        if key not in c:
            raise ValueError(f"missing campaign field: {key}")
    tests = []
    for t in c["tests"]:
        if "id" not in t or "action" not in t:
            raise ValueError("test missing id or action")
        tests.append(TestCase(
            id=t["id"],
            requirement=t.get("requirement"),
            action=t["action"],
            address=t.get("address", 0),
            count=t.get("count", 1),
            value=t.get("value", 0),
            inject=t.get("inject", {}) or {},
            expect=t.get("expect", {}) or {},
        ))
    return Campaign(
        id=c["id"],
        seed=c["seed"],
        target=c["target"],
        timeout_ms=c["timeout_ms"],
        tests=tests,
    )