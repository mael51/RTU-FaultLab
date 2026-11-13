import os
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
DUT = os.path.join(ROOT, "build", "rtufaultlab_dut_host.exe")
CLI = os.path.join(ROOT, "bench", "runner", "cli.py")


def test_campaign_loads():
    sys.path.insert(0, os.path.join(ROOT, "bench", "runner"))
    from campaign_loader import load_campaign
    c = load_campaign(os.path.join(ROOT, "bench", "campaigns", "smoke.yaml"))
    assert c.id == "modbus_smoke"
    assert len(c.tests) == 2


def test_smoke_campaign_runs():
    if not os.path.exists(DUT):
        import pytest
        pytest.skip("DUT not built")
    r = subprocess.run(
        [sys.executable, CLI, os.path.join(ROOT, "bench", "campaigns", "smoke.yaml"),
         "--dut", DUT],
        capture_output=True, text=True,
    )
    assert r.returncode == 0, r.stdout + r.stderr


def test_malformed_campaign_rejected():
    sys.path.insert(0, os.path.join(ROOT, "bench", "runner"))
    from campaign_loader import load_campaign
    import tempfile
    bad = os.path.join(tempfile.gettempdir(), "bad.yaml")
    with open(bad, "w") as f:
        f.write("foo: bar\n")
    try:
        load_campaign(bad)
        assert False, "should raise"
    except ValueError:
        pass