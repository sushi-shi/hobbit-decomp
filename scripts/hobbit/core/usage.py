"""hobbit.core.usage - best-effort command telemetry for the `hobbit` CLI.

Every CLI invocation appends one copyable line to build/hobbit_usage.log and
one `hobbit.usage.v1` record to build/hobbit_usage.jsonl:

    event_id, time (completion, UTC), started_at, duration_seconds,
    command, rc, outcome (success | difference | error), error_category,
    error (bounded stderr/stdout tail, failures only), revision, worktree,
    cwd, pid, is_test/test_marker (HOBBIT_USAGE_TEST=<marker>)

Routine output is never stored. Writing the log must not change a command's
return code. Records are written at completion, so a killed process leaves
none. Child processes started through `run_process` (ninja, and through it
cl) are teed so their diagnostics reach the record.
"""

from __future__ import annotations

import contextlib
import datetime
import json
import os
import re
import subprocess
import sys
import threading
import time
import traceback
import uuid
from pathlib import Path

TAIL = 16384


class _Tee:
    """Pass writes through to `stream` and keep the last TAIL characters."""

    def __init__(self, stream):
        self.stream = stream
        self.tail = ""

    def write(self, text):
        self.tail = (self.tail + text)[-TAIL:]
        try:
            return self.stream.write(text)
        except BrokenPipeError:
            return len(text)

    def flush(self):
        with contextlib.suppress(BrokenPipeError):
            self.stream.flush()

    def __getattr__(self, name):
        return getattr(self.stream, name)


def classify_error(text: str) -> str:
    """Best-effort category from diagnostic text; never inferred from rc alone."""
    lower = text.lower()
    if any(s in lower for s in ("operation not permitted", "permission denied")):
        return "environment.permission"
    if any(s in lower for s in ("winepath", "wineserver:", "wine: could not",
                                "wine: failed", "wineboot")):
        return "environment.wine"
    if re.search(r"\b(?:fatal )?error C\d{4}\b", text) or "produced no object" in lower:
        return "compile.cpp"
    if "no report" in lower and "hobbit build" in lower:
        return "build.stale"
    if "unknown target" in lower:
        return "build.target"
    if "traceback (most recent call last)" in lower:
        return "tool.exception"
    if any(s in lower for s in ("unrecognized arguments", "invalid choice",
                                "usage:", "unknown command", "unknown verb",
                                "expected one argument", "is not a unit")):
        return "cli.arguments"
    if "regression" in lower or ": fail" in lower:
        return "gate.failed"
    return "command.failed"


def run_logged(dispatch, argv: list[str], log_path: Path, *,
               failure_rc: int = 1) -> int:
    """Run `dispatch(argv)` and append its usage record to `log_path`."""
    started = time.monotonic()
    started_at = datetime.datetime.now(datetime.timezone.utc).isoformat()
    stdout, stderr = _Tee(sys.stdout), _Tee(sys.stderr)
    rc, error = 0, None
    try:
        with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
            rc = dispatch(argv) or 0
        return rc
    except SystemExit as exc:
        rc = exc.code if isinstance(exc.code, int) else (0 if exc.code is None else 1)
        if isinstance(exc.code, str):
            error = exc.code
        raise
    except BaseException as exc:
        rc = 130 if isinstance(exc, KeyboardInterrupt) else 2
        error = "".join(traceback.format_exception(type(exc), exc, exc.__traceback__))
        print(error, end="", file=sys.stderr)
        raise SystemExit(rc) from None
    finally:
        failed = rc < 0 or rc >= failure_rc or error is not None
        diagnostic = "\n".join(t for t in (stderr.tail.strip(), stdout.tail.strip()) if t)
        error = (error or diagnostic)[-TAIL:] if failed else None
        category = (("interrupted" if rc == 130 else "process.signal" if rc < 0
                     else classify_error(error or "")) if failed else None)
        append(log_path, argv, rc, started_at=started_at,
               duration_seconds=round(time.monotonic() - started, 3),
               outcome="error" if failed else "difference" if rc else "success",
               error_category=category, error=error)


def run_process(argv: list[str], *, cwd: Path) -> int:
    """Run a child with both pipes streamed through the current sys streams,
    so a logged invocation sees the child's diagnostics."""
    with subprocess.Popen(argv, cwd=cwd, stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE, text=True,
                          errors="replace") as process:
        def forward(pipe, stream):
            try:
                for line in pipe:
                    with contextlib.suppress(BrokenPipeError):
                        stream.write(line)
                        stream.flush()
            finally:
                pipe.close()
        threads = [threading.Thread(target=forward, args=pair, daemon=True)
                   for pair in ((process.stdout, sys.stdout),
                                (process.stderr, sys.stderr))]
        for thread in threads:
            thread.start()
        try:
            rc = process.wait()
        finally:
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()
            for thread in threads:
                thread.join()
        return rc


def append(log_path: Path, argv: list[str], rc: int, **fields) -> None:
    import shlex
    try:
        now = datetime.datetime.now(datetime.timezone.utc)
        root = log_path.parent.parent
        git = subprocess.run(["git", "rev-parse", "HEAD"], cwd=root,
                             capture_output=True, text=True, timeout=2)
        marker = os.environ.get("HOBBIT_USAGE_TEST") or None
        command = shlex.join(["hobbit", *argv])
        record = {"schema": "hobbit.usage.v1", "event_id": str(uuid.uuid4()),
                  "time": now.isoformat(), "command": command, "rc": rc,
                  "revision": git.stdout.strip() or None,
                  "worktree": str(root), "cwd": str(Path.cwd()),
                  "pid": os.getpid(), "is_test": marker is not None,
                  "test_marker": marker, **fields}
        log_path.parent.mkdir(parents=True, exist_ok=True)
        with log_path.open("a") as f:
            f.write(f"[{now:%Y-%m-%d}][{now:%H:%M:%S}][{rc}]: {command}\n")
        with log_path.with_suffix(".jsonl").open("a") as f:
            f.write(json.dumps(record, ensure_ascii=True) + "\n")
    except Exception:
        pass
