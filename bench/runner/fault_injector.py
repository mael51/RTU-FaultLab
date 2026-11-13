"""Deterministic fault injection engine for RTU-FaultLab."""

import random
from dataclasses import dataclass, field
from typing import List, Optional


@dataclass
class FaultConfig:
    seed: int
    drop: bool = False
    delay_ms: int = 0
    duplicate: bool = False
    corrupt_crc: bool = False
    truncate: bool = False


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
        })

    def apply(self, frame_hex: str) -> List[str]:
        """Return list of (possibly mutated) frames as hex strings."""
        results: List[str] = []
        bytes_list = bytes.fromhex(frame_hex.replace(" ", ""))

        if self.config.drop:
            self.log.mutations.append("drop")
            return []  # frame dropped

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
        frame_out = " ".join(frame_out[i:i+2] for i in range(0, len(frame_out), 2))
        results.append(frame_out)

        if self.config.duplicate:
            results.append(frame_out)
            self.log.mutations.append("duplicate")

        if self.config.delay_ms > 0:
            self.log.mutations.append(f"delay:{self.config.delay_ms}ms")

        return results
