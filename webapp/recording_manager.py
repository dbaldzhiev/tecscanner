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
        self._log_handle = None
        self.current_file: Optional[Path] = None
        self.current_log: Optional[Path] = None
        self.log_file = self.output_dir / "recordings.json"
        if not self.log_file.exists():
            self.log_file.write_text("[]")
        # Allow overriding the command used to invoke the recorder.
        self.record_cmd = os.getenv("LIVOX_RECORD_CMD", "save_laz")

    # ---- internal helpers -------------------------------------------------
    def _save_log(self, entry):
        data = json.loads(self.log_file.read_text())
        data.append(entry)
        self.log_file.write_text(json.dumps(data, indent=2))

    # ---- public API -------------------------------------------------------
    def start_recording(self) -> bool:
        """Start a Livox recording.

        Returns ``True`` if the recording process was successfully spawned and
        ``False`` if a recording is already running or if launching the
        external process fails.
        """

        if self._process is not None:
            return False
        timestamp = datetime.utcnow().strftime("%Y%m%d_%H%M%S")
        self.current_file = self.output_dir / f"recording_{timestamp}.laz"
        self.current_log = self.output_dir / f"recording_{timestamp}.log"
        cmd = [self.record_cmd, str(self.current_file)]
        logging.info("Starting recorder: %s", " ".join(cmd))
        try:
            log_handle = open(self.current_log, "w")
            self._process = subprocess.Popen(
                cmd, stdout=log_handle, stderr=subprocess.STDOUT
            )
            self._log_handle = log_handle
        except OSError:
            # Failed to start external recorder
            if self.current_log and self.current_log.exists():
                self.current_log.unlink()
            self.current_file = None
            return False
        return True

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
            "log": self.current_log.name if self.current_log else None,
            "stopped": datetime.utcnow().isoformat(),
        }
        self._save_log(entry)
        if self._log_handle:
            self._log_handle.close()
            self._log_handle = None
        self._process = None
        self.current_file = None
        self.current_log = None
        return True

    def status(self):
        return {
            "recording": self._process is not None,
            "current_file": self.current_file.name if self.current_file else None,
            "log_file": self.current_log.name if self.current_log else None,
        }

    def list_recordings(self):
        return json.loads(self.log_file.read_text())

    def get_log(self, name: str) -> Optional[str]:
        path = self.output_dir / name
        if path.is_file():
            return path.read_text()
        return None
