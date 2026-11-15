"""Virtual transport that drives the C++ DUT host process."""

import os
import subprocess
import threading
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
        # Consume stderr in background to avoid pipe deadlock
        self._stderr_lines = []
        self._stderr_thread = threading.Thread(target=self._read_stderr, daemon=True)
        self._stderr_thread.start()

    def _read_stderr(self):
        if self.proc.stderr is None:
            return
        for line in self.proc.stderr:
            self._stderr_lines.append(line.strip())

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
        self._stderr_lines = []
        self._stderr_thread = threading.Thread(target=self._read_stderr, daemon=True)
        self._stderr_thread.start()

    def send_frame(self, frame_hex: str, timeout_ms: int = 250, delay_ms: int = 0) -> Optional[str]:
        if delay_ms and delay_ms > 0:
            time.sleep(delay_ms / 1000.0)

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
            time.sleep(0.02)
            return line[5:]
        return None

    def close(self):
        try:
            if self.proc.stdin:
                self.proc.stdin.close()
            self.proc.wait(timeout=5)
        except Exception:
            self.proc.kill()

    @property
    def stderr_lines(self):
        return list(self._stderr_lines)