import os
import subprocess
import sys
import threading
import time

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


def _crc(data):
    c = 0xFFFF
    for b in data:
        c ^= b
        for _ in range(8):
            if c & 1:
                c = (c >> 1) ^ 0xA001
            else:
                c >>= 1
    return c


def test_safety_timeout_integration():
    # TC-SAFETY-004: DUT heartbeat timeout integration
    if not os.path.exists(DUT):
        import pytest
        pytest.skip("DUT not built")
    env = os.environ.copy()
    env["PATH"] = r"C:\msys64\mingw64\bin;" + env.get("PATH", "")
    proc = subprocess.Popen(
        [DUT],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1,
        env=env,
    )
    lines = []

    def reader():
        for l in proc.stdout:
            lines.append(l.strip())

    t = threading.Thread(target=reader, daemon=True)
    t.start()

    def send(frame_hex):
        proc.stdin.write(f"REQ {frame_hex}\n")
        proc.stdin.flush()

    # enable output 1 (activates outputs)
    d = [0x01, 0x06, 0x00, 0x00, 0x00, 0x01]
    c = _crc(d)
    send("01 06 00 00 00 01 {:02X} {:02X}".format(c & 0xFF, (c >> 8) & 0xFF))
    time.sleep(0.2)
    # heartbeat (transitions READY -> RUNNING)
    d2 = [0x01, 0x06, 0x00, 0x05, 0x00, 0x01]
    c2 = _crc(d2)
    send("01 06 00 05 00 01 {:02X} {:02X}".format(c2 & 0xFF, (c2 >> 8) & 0xFF))
    time.sleep(1.5)
    # read output state (should be 0 after timeout)
    d3 = [0x01, 0x03, 0x00, 0x01, 0x00, 0x01]
    c3 = _crc(d3)
    send("01 03 00 01 00 01 {:02X} {:02X}".format(c3 & 0xFF, (c3 >> 8) & 0xFF))
    time.sleep(0.5)
    proc.stdin.close()
    t.join(timeout=2)
    proc.terminate()
    # find the last RESP for output state
    pass
    pass
    resp = None
    for l in reversed(lines):
        if l.startswith("RESP ") and len(l.split()) >= 5:
            resp = l
            break
    if resp is None:
        resp = lines[-1] if lines else ""
    parts = resp.split()
    # strip "RESP" prefix
    mp = parts[1:]
    assert len(mp) >= 5, f"bad response: {resp}"
    val = (int(mp[3], 16) << 8) | int(mp[4], 16)
    assert val == 0, f"expected 0, got {val}"