"""Utilities for managing Livox recordings.

The original implementation in this repository merely simulated a LiDAR
device by periodically writing mock "point" entries to a file.  This patch
replaces those sections with real calls to the Livox SDK.  The implementation
follows the approach used by the
`mandeye_controller <https://github.com/JanuszBedkowski/mandeye_controller>`_
project: an external Livox recording binary is spawned which streams data
directly to a ``.laz`` file.  The recording process is managed with
``subprocess`` and terminated when recording stops.

The path to the Livox recorder executable can be configured via the
``LIVOX_RECORD_CMD`` environment variable.  It should point to a command that
accepts the desired output filename as its last argument and records until it
receives ``SIGINT``.
"""

import json
import os
import signal
import subprocess
from typing import Optional
from datetime import datetime
from pathlib import Path
import logging

logger = logging.getLogger(__name__)

class RecordingManager:
    """Manage MID360 recordings by delegating to the Livox SDK.

    The manager spawns an external recorder process (typically the
    ``save_laz`` utility from ``mandeye_controller``) and tracks the produced
    file.  The process is started when ``start_recording`` is called and is
    stopped via ``SIGINT`` when ``stop_recording`` is requested.
    """

    def __init__(self, output_dir: str = "recordings"):
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        self._process: Optional[subprocess.Popen] = None
        self.current_file: Optional[Path] = None
        self.log_file = self.output_dir / "recordings.json"
        if not self.log_file.exists():
            self._write_log([])
        # Allow overriding the command used to invoke the recorder.
        self.record_cmd = os.getenv("LIVOX_RECORD_CMD", "save_laz")

    # ---- internal helpers -------------------------------------------------
    def _load_log(self):
        try:
            return json.loads(self.log_file.read_text())
        except json.JSONDecodeError:
            logger.error("Corrupted recordings log detected; resetting")
            self._write_log([])
            return []

    def _write_log(self, data):
        tmp_path = self.log_file.with_name(self.log_file.name + ".tmp")
        tmp_path.write_text(json.dumps(data, indent=2))
        os.replace(tmp_path, self.log_file)

    def _save_log(self, entry):
        data = self._load_log()
        data.append(entry)
        self._write_log(data)

    # ---- public API -------------------------------------------------------
    def start_recording(self) -> tuple[bool, Optional[str]]:
        """Start a Livox recording.

        Returns a tuple ``(started, error)`` where ``started`` indicates
        whether the external recorder process was launched and ``error`` is
        ``None`` on success or a string identifying the failure.  Possible
        error codes are ``"already_active"`` when a recording is in progress
        and ``"spawn_failed"`` when the recorder process cannot be created.
        """

        if self._process is not None:
            # A recording is already running
            return False, "already_active"
        timestamp = datetime.utcnow().strftime("%Y%m%d_%H%M%S")
        self.current_file = self.output_dir / f"recording_{timestamp}.laz"
        cmd = [self.record_cmd, str(self.current_file)]
        try:
            self._process = subprocess.Popen(cmd)
        except OSError:
            # Failed to start external recorder
            self.current_file = None
            return False, "spawn_failed"
        return True, None

    def stop_recording(self) -> bool:
        """Stop the Livox recording and log the result."""

        if self._process is None:
            return False
        # Politely ask the process to terminate; fall back to kill.
        self._process.send_signal(signal.SIGINT)
        try:
            self._process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            self._process.kill()
            self._process.wait()
        entry = {
            "file": self.current_file.name,
            "stopped": datetime.utcnow().isoformat()
        }
        self._save_log(entry)
        self._process = None
        self.current_file = None
        return True

    def status(self):
        if self._process is not None and self._process.poll() is not None:
            self._process = None
            self.current_file = None
        return {
            "recording": self._process is not None,
            "current_file": self.current_file.name if self.current_file else None,
        }

    def list_recordings(self):
        return self._load_log()
