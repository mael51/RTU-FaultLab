"""Deterministic fault injection engine for RTU-FaultLab.

Injection semantics (V1-B4) — every fault declares its point of application:
- TX (request, PC -> DUT): drop, delay_ms, duplicate, corrupt_crc, truncate
- RX (response, DUT -> PC): drop_response, delay_response_ms

The runner is responsible for applying RX faults; the injector only declares
them and logs the configuration.
"""

import random
from dataclasses import dataclass, field
from typing import List, Optional


@dataclass
class FaultConfig:
    seed: int
    # --- TX faults (applied to the request frame) ---
    drop: bool = False
    delay_ms: int = 0
    duplicate: bool = False
    corrupt_crc: bool = False
    truncate: bool = False
    # --- RX faults (applied to the response by the runner) ---
    drop_response: bool = False
    delay_response_ms: int = 0

    def tx_faults(self) -> List[str]:
        return [k for k in ("drop", "duplicate", "corrupt_crc", "truncate")
                if getattr(self, k)]


@dataclass
class FaultLog:
    seed: int
    params: dict
    mutations: List[str] = field(default_factory=list)


class FaultInjector:
    def __init__(self, config: FaultConfig):
        self.config = config
        self.rng = random.Random(config.seed)
        self.log = FaultLog(seed=config.seed, params={
            "drop": config.drop,
            "delay_ms": config.delay_ms,
            "duplicate": config.duplicate,
            "corrupt_crc": config.corrupt_crc,
            "truncate": config.truncate,
            "drop_response": config.drop_response,
            "delay_response_ms": config.delay_response_ms,
        })

    def apply(self, frame_hex: str) -> List[str]:
        """Return list of (possibly mutated) request frames as hex strings."""
        results: List[str] = []
        bytes_list = bytes.fromhex(frame_hex.replace(" ", ""))

        if self.config.drop:
            self.log.mutations.append("drop")
            return []  # frame dropped (TX)

        if self.config.truncate and len(bytes_list) > 2:
            cut = self.rng.randint(1, len(bytes_list) - 1)
            bytes_list = bytes_list[:cut]
            self.log.mutations.append(f"truncate:{cut}")

        if self.config.corrupt_crc and len(bytes_list) >= 2:
            # flip a bit in the last byte (CRC low)
            b = bytearray(bytes_list)
            b[-1] ^= 0xFF
            bytes_list = bytes(b)
            self.log.mutations.append("corrupt_crc")

        frame_out = bytes_list.hex().upper()
        # format with spaces for readability
        frame_out = " ".join(frame_out[i:i + 2] for i in range(0, len(frame_out), 2))
        results.append(frame_out)

        if self.config.duplicate:
            results.append(frame_out)
            self.log.mutations.append("duplicate")

        if self.config.delay_ms > 0:
            self.log.mutations.append(f"delay:{self.config.delay_ms}ms")

        # RX faults are declared/logged here, applied by the runner
        if self.config.drop_response:
            self.log.mutations.append("drop_response")
        if self.config.delay_response_ms > 0:
            self.log.mutations.append(f"delay_response:{self.config.delay_response_ms}ms")

        return results
