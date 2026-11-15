"""Unit tests for the bench runner: oracle, fault injector, traceability.

Covers STAB-E02 (injection semantics), STAB-F02 (oracle hardening) and
STAB-F03 (traceability statuses).
"""

import os
import sys
import textwrap

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "bench", "runner"))

from oracle import check_crc, check_expect  # noqa: E402
from fault_injector import FaultConfig, FaultInjector  # noqa: E402
from traceability import build_traceability, traceability_violations  # noqa: E402


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def crc16(data: bytes) -> int:
    crc = 0xFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
    return crc


def frame(slave=0x01, fn=0x03, addr=0x0001, value=0x0001) -> str:
    payload = bytes([slave, fn, (addr >> 8) & 0xFF, addr & 0xFF,
                     (value >> 8) & 0xFF, value & 0xFF])
    c = crc16(payload)
    full = payload + bytes([c & 0xFF, (c >> 8) & 0xFF])
    return " ".join(f"{b:02X}" for b in full)


def response(fn=0x03, data=b"\x00\x01", exception=None) -> str:
    if exception is None:
        payload = bytes([0x01, fn, len(data)]) + data
    else:
        payload = bytes([0x01, fn | 0x80, exception])
    c = crc16(payload)
    full = payload + bytes([c & 0xFF, (c >> 8) & 0xFF])
    return " ".join(f"{b:02X}" for b in full)


# ---------------------------------------------------------------------------
# Oracle
# ---------------------------------------------------------------------------

def test_oracle_crc_ok():
    assert check_crc(response())


def test_oracle_crc_bad():
    r = response()
    parts = r.split()
    parts[-1] = f"{int(parts[-1], 16) ^ 0xFF:02X}"
    assert not check_crc(" ".join(parts))


def test_oracle_success_rejects_exception_response():
    exc_resp = response(exception=0x02)
    assert not check_expect({"status": "success"}, exc_resp, {}, False)


def test_oracle_success_rejects_bad_crc():
    r = response()
    parts = r.split()
    parts[-2] = f"{int(parts[-2], 16) ^ 0xFF:02X}"
    assert not check_expect({"status": "success"}, " ".join(parts), {}, False)


def test_oracle_exception_wrong_code():
    assert not check_expect({"exception": 0x03}, response(exception=0x02), {}, False)
    assert check_expect({"exception": 0x02}, response(exception=0x02), {}, False)


def test_oracle_expected_timeout():
    assert check_expect({"status": "timeout"}, None, {}, True)
    assert not check_expect({"status": "timeout"}, response(), {}, False)


def test_oracle_success_rejects_timeout():
    assert not check_expect({"status": "success"}, None, {}, True)


def test_oracle_register_values():
    r = response(fn=0x03, data=b"\x00\x01\x00\x02")
    assert check_expect({"register_values": [1, 2]}, r, {}, False)
    assert not check_expect({"register_values": [1, 3]}, r, {}, False)


def test_oracle_echo_value():
    r = frame(fn=0x06, addr=0x0000, value=0x0001)
    assert check_expect({"echo_value": 1}, r, {}, False)
    assert not check_expect({"echo_value": 2}, r, {}, False)


def test_oracle_dut_output_state():
    assert check_expect({"dut_output_state": 3}, None, {"output_state": 3}, False)
    assert not check_expect({"dut_output_state": 3}, None, {"output_state": 0}, False)


# ---------------------------------------------------------------------------
# Fault injector (STAB-E02)
# ---------------------------------------------------------------------------

def test_injector_drop():
    fi = FaultInjector(FaultConfig(seed=1, drop=True))
    assert fi.apply(frame()) == []
    assert "drop" in fi.log.mutations


def test_injector_corrupt_crc():
    fi = FaultInjector(FaultConfig(seed=1, corrupt_crc=True))
    out = fi.apply(frame())
    assert len(out) == 1
    assert not check_crc(out[0])  # frame CRC is now invalid
    assert "corrupt_crc" in fi.log.mutations


def test_injector_truncate():
    fi = FaultInjector(FaultConfig(seed=42, truncate=True))
    out = fi.apply(frame())
    assert len(out) == 1
    assert "truncate" in fi.log.mutations[0]


def test_injector_duplicate():
    fi = FaultInjector(FaultConfig(seed=1, duplicate=True))
    out = fi.apply(frame())
    assert len(out) == 2 and out[0] == out[1]


def test_injector_deterministic():
    f1 = FaultInjector(FaultConfig(seed=7, truncate=True)).apply(frame())
    f2 = FaultInjector(FaultConfig(seed=7, truncate=True)).apply(frame())
    assert f1 == f2  # same seed -> same mutation (STAB-E03)


def test_injector_delay_logged():
    fi = FaultInjector(FaultConfig(seed=1, delay_ms=150))
    out = fi.apply(frame())
    assert len(out) == 1
    assert "delay:150ms" in fi.log.mutations


# ---------------------------------------------------------------------------
# Traceability (STAB-F03)
# ---------------------------------------------------------------------------

def _write_requirements(tmp_path, content):
    p = tmp_path / "requirements.yaml"
    p.write_text(textwrap.dedent(content), encoding="utf-8")
    return str(p)


def test_traceability_statuses(tmp_path):
    req = _write_requirements(tmp_path, """
        requirements:
          FR-A: { description: a, tests: [T1, T2], priority: MUST }
          FR-B: { description: b, tests: [T3], priority: MUST }
          FR-C: { description: c, tests: [GHOST], priority: SHOULD }
        test_cases:
          T1: { requirement: FR-A, stimulus: s, expected: e }
          T2: { requirement: FR-A, stimulus: s, expected: e }
          T3: { requirement: FR-B, stimulus: s, expected: e }
    """)
    results = [{"id": "T1", "passed": True}, {"id": "T3", "passed": False}]
    matrix = build_traceability(req, results, str(tmp_path / "m.json"))
    assert matrix["FR-A"]["overall"] == "COVERED_PASS"
    assert matrix["FR-A"]["statuses"]["T2"] == "NOT_EXECUTED"
    assert matrix["FR-B"]["overall"] == "COVERED_FAIL"
    assert matrix["FR-C"]["overall"] == "INVALID_REFERENCE"


def test_traceability_must_without_tests_blocks(tmp_path):
    req = _write_requirements(tmp_path, """
        requirements:
          FR-X: { description: x, tests: [], priority: MUST }
        test_cases: {}
    """)
    matrix = build_traceability(req, [], str(tmp_path / "m.json"))
    assert matrix["FR-X"]["overall"] == "NOT_COVERED"
    assert traceability_violations(matrix)


def test_traceability_violations_not_executed_not_blocking(tmp_path):
    req = _write_requirements(tmp_path, """
        requirements:
          FR-Y: { description: y, tests: [T9], priority: MUST }
        test_cases:
          T9: { requirement: FR-Y, stimulus: s, expected: e }
    """)
    matrix = build_traceability(req, [], str(tmp_path / "m.json"))
    assert matrix["FR-Y"]["overall"] == "NOT_EXECUTED"
    assert not traceability_violations(matrix)
