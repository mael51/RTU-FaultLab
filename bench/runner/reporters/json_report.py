"""Generate JSON report."""

import json


def write_json(campaign_id: str, seed: int, results: list, path: str):
    data = {
        "campaign": campaign_id,
        "seed": seed,
        "results": results,
    }
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)