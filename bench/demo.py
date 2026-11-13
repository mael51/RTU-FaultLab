"""Demo script: run a nominal campaign and a fault-injected campaign."""

import os
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
DUT = os.path.join(ROOT, "build", "rtufaultlab_dut_host.exe")
CLI = os.path.join(ROOT, "bench", "runner", "cli.py")

campaigns = [
    ("Nominal (smoke)", os.path.join(ROOT, "bench", "campaigns", "smoke.yaml")),
    ("Robustness (faults)", os.path.join(ROOT, "bench", "campaigns", "robustness.yaml")),
]

for name, camp in campaigns:
    print(f"\n=== {name} ===")
    r = subprocess.run(
        [sys.executable, CLI, camp, "--dut", DUT],
        capture_output=True, text=True,
    )
    print(r.stdout.strip())
    if r.stderr:
        print("stderr:", r.stderr.strip())
    print(f"exit code: {r.returncode}")