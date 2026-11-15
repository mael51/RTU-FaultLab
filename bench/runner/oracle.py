"""Oracle checks for campaign test results."""

import struct


def _crc16(data: bytes) -> int:
    crc = 0xFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
    return crc


def check_crc(response_hex: str) -> bool:
    parts = response_hex.split()
    if len(parts) < 3:
        return False
    data = bytes(int(p, 16) for p in parts[:-2])
    crc = int(parts[-2], 16) | (int(parts[-1], 16) << 8)
    return crc == _crc16(data)


def check_expect(expect: dict, response_hex: str, dut_state: dict, timed_out: bool) -> bool:
    if "status" in expect:
        if expect["status"] == "success":
            if response_hex is None or timed_out:
                return False
            if not check_crc(response_hex):
                return False
        if expect["status"] == "timeout":
            if not timed_out:
                return False
    if "exception" in expect:
        if response_hex is None:
            return False
        parts = response_hex.split()
        if len(parts) < 3:
            return False
        if not (int(parts[1], 16) & 0x80):
            return False
        if int(parts[2], 16) != expect["exception"]:
            return False
    if "dut_output_state" in expect:
        expected = expect["dut_output_state"]
        if isinstance(expected, str):
            expected = int(expected, 16) if expected.startswith("0x") else int(expected)
        if dut_state.get("output_state") != expected:
            return False
    return True