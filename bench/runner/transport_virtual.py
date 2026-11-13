"""Virtual transport that drives the C++ DUT host process."""

import os
import subprocess
import time
from typing import Optional


class VirtualTransport:
    def __init__(self, dut_exe: str):
        self._exe = dut_exe
        env = os.environ.copy()
        env["PATH"] = r"C:\msys64\mingw64\bin;" + env.get("PATH", "")
        self.proc = subprocess.Popen(
            [dut_exe],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1,
            env=env,
        )

    def _restart(self):
        self.close()
        env = os.environ.copy()
        env["PATH"] = r"C:\msys64\mingw64\bin;" + env.get("PATH", "")
        self.proc = subprocess.Popen(
            [self._exe],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1,
            env=env,
        )

    def send_frame(self, frame_hex: str, timeout_ms: int = 250) -> Optional[str]:
        if self.proc.poll() is not None:
            self._restart()
        assert self.proc.stdin is not None
        try:
            self.proc.stdin.write(f"REQ {frame_hex}\n")
            self.proc.stdin.flush()
        except (OSError, BrokenPipeError):
            self._restart()
            assert self.proc.stdin is not None
            self.proc.stdin.write(f"REQ {frame_hex}\n")
            self.proc.stdin.flush()
        # read response with timeout
        import select
        # Windows: select on pipes not supported; use readline with timeout via thread
        import threading

        result = {"line": None}

        def reader():
            if self.proc.stdout is not None:
                result["line"] = self.proc.stdout.readline()

        t = threading.Thread(target=reader, daemon=True)
        t.start()
        t.join(timeout=timeout_ms / 1000.0)
        if t.is_alive():
            return None  # timeout
        line = result["line"]
        if line is None:
            return None
        line = line.strip()
        if line.startswith("RESP "):
            time.sleep(0.05)
            return line[5:]
        return None

    def close(self):
        try:
            if self.proc.stdin:
                self.proc.stdin.close()
            self.proc.wait(timeout=5)
        except Exception:
            self.proc.kill()