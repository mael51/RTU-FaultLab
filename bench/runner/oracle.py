"""Oracle checks for campaign test results (STAB-F02).

The oracle verifies the response content itself, not just its presence:
CRC, slave address, function code, exception code, FC06 echo value and
the DUT output state. It must fail on any deliberately altered response.
"""

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


def parse_response(response_hex: str):
    """Return (data_bytes, received_crc) or None if malformed."""
    parts = response_hex.split()
    if len(parts) < 3:
        return None
    try:
        data = bytes(int(p, 16) for p in parts[:-2])
        crc = int(parts[-2], 16) | (int(parts[-1], 16) << 8)
    except ValueError:
        return None
    return data, crc


def check_crc(response_hex: str) -> bool:
    parsed = parse_response(response_hex)
    if parsed is None:
        return False
    data, crc = parsed
    return crc == _crc16(data)


def check_expect(expect: dict, response_hex: str, dut_state: dict,
                 timed_out: bool, request_hex: str = None) -> bool:
    # --- expected timeout -------------------------------------------------
    if expect.get("status") == "timeout":
        return timed_out
    if timed_out and expect.get("status") != "timeout":
        return False

    # --- expected success -------------------------------------------------
    if expect.get("status") == "success":
        if response_hex is None:
            return False
        if not check_crc(response_hex):
            return False
        parsed = parse_response(response_hex)
        if parsed is None:
            return False
        data, _ = parsed
        if len(data) < 2:
            return False
        if data[1] & 0x80:
            return False  # success expected, got exception

    # --- expected exception ------------------------------------------------
    if "exception" in expect:
        if response_hex is None:
            return False
        if not check_crc(response_hex):
            return False  # exceptions must also carry a valid CRC
        parsed = parse_response(response_hex)
        if parsed is None:
            return False
        data, _ = parsed
        if len(data) < 3:
            return False
        if not (data[1] & 0x80):
            return False
        if data[2] != expect["exception"]:
            return False

    # --- expected slave address ---------------------------------------------
    if "slave" in expect and response_hex is not None:
        parsed = parse_response(response_hex)
        if parsed is None:
            return False
        data, _ = parsed
        if not data or data[0] != int(expect["slave"]):
            return False

    # --- expected function code ---------------------------------------------
    if "function" in expect and response_hex is not None:
        parsed = parse_response(response_hex)
        if parsed is None:
            return False
        data, _ = parsed
        if len(data) < 2 or data[1] != int(expect["function"]):
            return False

    # --- expected FC06 echo value (bytes 4..5) -------------------------------
    if "echo_value" in expect and response_hex is not None:
        parsed = parse_response(response_hex)
        if parsed is None:
            return False
        data, _ = parsed
        if len(data) < 6:
            return False
        echoed = (data[4] << 8) | data[5]
        if echoed != int(expect["echo_value"]):
            return False

    # --- expected FC03 register value(s) -------------------------------------
    if "register_values" in expect and response_hex is not None:
        parsed = parse_response(response_hex)
        if parsed is None:
            return False
        data, _ = parsed
        if len(data) < 3 or data[1] != 0x03:
            return False
        byte_count = data[2]
        if byte_count != len(data) - 3:  # slave + func + count + N data bytes
            return False
        expected = expect["register_values"]
        if isinstance(expected, int):
            expected = [expected]
        got = [(data[3 + i * 2] << 8) | data[4 + i * 2]
               for i in range(byte_count // 2)]
        if got != list(expected):
            return False

    # --- expected DUT output state -------------------------------------------
    if "dut_output_state" in expect:
        expected = expect["dut_output_state"]
        if isinstance(expected, str):
            expected = int(expected, 16) if expected.startswith("0x") else int(expected)
        got = dut_state.get("output_state")
        if got is None:
            # state was never read: an assertion on it can never PASS (V1-B3)
            return False
        if got != expected:
            return False

    return True
