"""CLI runner for RTU-FaultLab campaigns."""

import argparse
import json
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))

from campaign_loader import load_campaign
from fault_injector import FaultInjector, FaultConfig
from transport_virtual import VirtualTransport
from oracle import check_expect
from reporters.junit import write_junit
from reporters.json_report import write_json
from reporters.html_report import write_html
from traceability import build_traceability


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


def build_frame(slave: int, action: str, address: int, value: int = 0, count: int = 1) -> str:
    if action == "read_holding_registers":
        payload = bytes([slave, 0x03, (address >> 8) & 0xFF, address & 0xFF,
                        (count >> 8) & 0xFF, count & 0xFF])
    elif action == "write_single_register":
        payload = bytes([slave, 0x06, (address >> 8) & 0xFF, address & 0xFF,
                        (value >> 8) & 0xFF, value & 0xFF])
    else:
        raise ValueError(f"unknown action: {action}")
    c = crc16(payload)
    full = payload + bytes([c & 0xFF, (c >> 8) & 0xFF])
    return " ".join(f"{b:02X}" for b in full)


def main():
    parser = argparse.ArgumentParser(description="RTU-FaultLab campaign runner")
    parser.add_argument("campaign", help="path to campaign YAML")
    parser.add_argument("--dut", default=os.path.join("build", "rtufaultlab_dut_host.exe"),
                        help="path to DUT host executable")
    parser.add_argument("--out", default=None, help="JSON result output path")
    parser.add_argument("--reports", default=None, help="directory for JUnit/JSON/HTML reports")
    parser.add_argument("--requirements", default=os.path.join(os.path.dirname(__file__), "..", "traceability", "requirements.yaml"),
                        help="requirements yaml for traceability")
    args = parser.parse_args()

    camp = load_campaign(args.campaign)
    transport = VirtualTransport(args.dut)

    results = []
    all_pass = True
    for tc in camp.tests:
        frame = build_frame(1, tc.action, tc.address, tc.value, tc.count)

        # fault injection
        cfg = FaultConfig(seed=camp.seed, **{k: v for k, v in tc.inject.items()
                                            if k in ("drop", "delay_ms", "duplicate",
                                                     "corrupt_crc", "truncate")})
        # normalize inject keys
        inj = {}
        if "corrupt_crc" in tc.inject:
            inj["corrupt_crc"] = bool(tc.inject["corrupt_crc"])
        if "drop" in tc.inject:
            inj["drop"] = bool(tc.inject["drop"])
        if "duplicate" in tc.inject:
            inj["duplicate"] = bool(tc.inject["duplicate"])
        if "truncate" in tc.inject:
            inj["truncate"] = bool(tc.inject["truncate"])
        if "delay_ms" in tc.inject:
            inj["delay_ms"] = int(tc.inject["delay_ms"])
        cfg = FaultConfig(seed=camp.seed, **inj)

        fi = FaultInjector(cfg)
        frames = fi.apply(frame)

        timed_out = False
        response_hex = None
        # send (possibly multiple) frames; last response wins
        for f in frames:
            if cfg.delay_ms and cfg.delay_ms > camp.timeout_ms:
                timed_out = True
                continue
            r = transport.send_frame(f, timeout_ms=camp.timeout_ms)
            if r is None:
                timed_out = True
            else:
                response_hex = r

        # dut state: read output state register
        state_frame = build_frame(1, "read_holding_registers", 0x0001, 0, 1)
        st_resp = transport.send_frame(state_frame, timeout_ms=camp.timeout_ms)
        dut_state = {"output_state": 0}
        if st_resp:
            parts = st_resp.split()
            if len(parts) >= 5:
                dut_state["output_state"] = (int(parts[3], 16) << 8) | int(parts[4], 16)

        ok = check_expect(tc.expect, response_hex, dut_state, timed_out)
        if not ok:
            all_pass = False
        results.append({
            "id": tc.id,
            "requirement": tc.requirement,
            "passed": ok,
            "response": response_hex,
            "timed_out": timed_out,
            "mutations": fi.log.mutations,
        })
        print(f"{tc.id}: {'PASS' if ok else 'FAIL'}")

    transport.close()

    if args.out:
        with open(args.out, "w", encoding="utf-8") as f:
            json.dump({"campaign": camp.id, "results": results}, f, indent=2)

    # Generate reports
    reports_dir = args.reports
    if reports_dir:
        os.makedirs(reports_dir, exist_ok=True)
        write_junit(results, os.path.join(reports_dir, "junit.xml"), suite_name=camp.id)
        write_json(camp.id, camp.seed, results, os.path.join(reports_dir, "results.json"))
        write_html(camp.id, results, os.path.join(reports_dir, "report.html"))
        req_path = os.path.abspath(args.requirements)
        if os.path.exists(req_path):
            build_traceability(req_path, results, os.path.join(reports_dir, "traceability.json"))

    sys.exit(0 if all_pass else 1)


if __name__ == "__main__":
    main()