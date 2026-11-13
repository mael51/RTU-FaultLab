"""Oracle checks for campaign test results."""


def check_expect(expect: dict, response_hex: str, dut_state: dict, timed_out: bool) -> bool:
    if "status" in expect:
        if expect["status"] == "success":
            if response_hex is None or timed_out:
                return False
        if expect["status"] == "timeout":
            if not timed_out:
                return False
    if "dut_output_state" in expect:
        expected = expect["dut_output_state"]
        if isinstance(expected, str):
            expected = int(expected, 16) if expected.startswith("0x") else int(expected)
        if dut_state.get("output_state") != expected:
            return False
    return True