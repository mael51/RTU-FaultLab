"""Virtual transport that drives the C++ DUT host process.

Design (STAB-E01):
- Exactly one persistent reader thread owns ``proc.stdout``; responses are
  pushed into a thread-safe queue and correlated sequentially by the caller.
- Exactly one persistent reader thread owns ``proc.stderr``.
- Timeouts use ``time.monotonic()``-based blocking queue reads (no busy sleep).
- Responses are timestamped at arrival so RX delays can be simulated faithfully.
- No hardcoded toolchain paths: the DUT inherits the current environment; pass
  a ``PATH``-prepared environment via the ``env`` argument if needed.
- Clean shutdown: terminate -> bounded wait -> kill -> join reader threads.
"""

import os
import queue
import subprocess
import threading
import time
from typing import List, Optional

_STOP = object()  # sentinel pushed on EOF so waiting callers wake up


class VirtualTransport:
    def __init__(self, dut_exe: str, env: Optional[dict] = None):
        self._exe = dut_exe
        self._env = env if env is not None else os.environ.copy()
        self._resp_q: "queue.Queue" = queue.Queue()
        self._stderr_lines: List[str] = []
        self._stderr_lock = threading.Lock()
        self._closed = False
        self._restart_count = 0
        self._resync_needed = False
        self._start_process()

    # ------------------------------------------------------------------ setup
    def _start_process(self) -> None:
        self.proc = subprocess.Popen(
            [self._exe],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1,
            env=self._env,
        )
        self._stdout_thread = threading.Thread(target=self._read_stdout, daemon=True)
        self._stderr_thread = threading.Thread(target=self._read_stderr, daemon=True)
        self._stdout_thread.start()
        self._stderr_thread.start()

    # ----------------------------------------------------------- reader loops
    def _read_stdout(self) -> None:
        assert self.proc.stdout is not None
        try:
            for line in self.proc.stdout:
                line = line.strip()
                if line.startswith("RESP "):
                    # Stamp with arrival time (monotonic) for delayed RX simulation.
                    self._resp_q.put((time.monotonic(), line[5:]))
        except (OSError, ValueError):
            pass  # stream closed during shutdown
        finally:
            self._resp_q.put(_STOP)

    def _read_stderr(self) -> None:
        if self.proc.stderr is None:
            return
        try:
            for line in self.proc.stderr:
                with self._stderr_lock:
                    self._stderr_lines.append(line.strip())
        except (OSError, ValueError):
            pass

    # ------------------------------------------------------------------ send
    def write_request(self, frame_hex: str) -> None:
        """Write one request envelope to the DUT stdin (no response read)."""
        if self.proc.poll() is not None:
            self._restart_if_dead()
        try:
            assert self.proc.stdin is not None
            self.proc.stdin.write(f"REQ {frame_hex}\n")
            self.proc.stdin.flush()
        except (OSError, BrokenPipeError):
            self._restart_if_dead()
            assert self.proc.stdin is not None
            self.proc.stdin.write(f"REQ {frame_hex}\n")
            self.proc.stdin.flush()

    def read_response(self, timeout_ms: int = 250, delay_ms: int = 0) -> Optional[str]:
        """Read one correlated response, or None on timeout (STAB-V1-B3).

        ``delay_ms`` simulates a RX delay: a response is only considered
        available ``delay_ms`` after its (timestamped) arrival.
        """
        if self._resync_needed:
            self._resync_needed = False
            self._purge_pending()

        deadline = time.monotonic() + timeout_ms / 1000.0
        while True:
            now = time.monotonic()
            if now >= deadline:
                self._resync_needed = True
                return None
            try:
                item = self._resp_q.get_nowait()
            except queue.Empty:
                time.sleep(0.005)
                continue
            if item is _STOP:
                return None
            arrival, payload = item
            ready_at = arrival + delay_ms / 1000.0
            if now >= ready_at:
                return payload
            # not ready yet: put back and wait
            self._resp_q.put(item)
            wait = min(0.01, ready_at - now, deadline - now)
            if wait <= 0:
                self._resync_needed = True
                return None
            time.sleep(wait)

    def _purge_pending(self) -> None:
        """Drop queued responses from an earlier timed-out request."""
        dropped = 0
        while True:
            try:
                item = self._resp_q.get_nowait()
            except queue.Empty:
                break
            if item is _STOP:
                continue
            dropped += 1
        if dropped:
            with self._stderr_lock:
                self._stderr_lines.append(
                    f"transport: purged {dropped} late response(s) after timeout"
                )

    def discard_next_response(self) -> None:
        """Declare the next (possibly late) response as lost; purge on next read."""
        self._resync_needed = True

    def send_frame(self, frame_hex: str, timeout_ms: int = 250,
                   delay_ms: int = 0) -> Optional[str]:
        """Send one request frame; return the response hex (or None on timeout)."""
        if delay_ms and delay_ms > 0:
            time.sleep(delay_ms / 1000.0)
        self.write_request(frame_hex)
        return self.read_response(timeout_ms, delay_ms=delay_ms)

    def close(self) -> None:
        if self._closed:
            return
        self._closed = True
        self._shutdown_process()

    # ----------------------------------------------------------- restart/close
    def _restart_if_dead(self) -> None:
        self._restart_count += 1
        if self._restart_count > 3:
            raise RuntimeError(
                f"DUT process '{self._exe}' keeps dying (exit code "
                f"{self.proc.returncode}). Check that it can start in the "
                "current environment (PATH/DLLs) before running the campaign."
            )
        self.restart()

    def restart(self) -> None:
        self._restart_count = 0
        self._resync_needed = False
        self._shutdown_process()
        self._resp_q = queue.Queue()
        self._stderr_lines = []
        self._closed = False
        self._start_process()

    def _shutdown_process(self) -> None:
        if getattr(self, "proc", None) is None:
            return
        try:
            if self.proc.stdin is not None:
                self.proc.stdin.close()
        except (OSError, ValueError):
            pass
        try:
            self.proc.terminate()
            self.proc.wait(timeout=5)
        except Exception:
            self.proc.kill()
            try:
                self.proc.wait(timeout=5)
            except Exception:
                pass
        # Drain sentinel then join readers (they exit on closed streams).
        try:
            self._resp_q.get_nowait()
        except queue.Empty:
            pass
        for t in (self._stdout_thread, self._stderr_thread):
            t.join(timeout=2)

    @property
    def stderr_lines(self) -> List[str]:
        with self._stderr_lock:
            return list(self._stderr_lines)