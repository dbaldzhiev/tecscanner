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
from typing import Optional, List
from datetime import datetime
from pathlib import Path
import logging

logger = logging.getLogger(__name__)

class RecordingManager:
    """Manage MID360 recordings by delegating to the Livox SDK.

    The manager spawns an external recorder process (typically the
    ``save_laz`` utility from ``mandeye_controller``) and tracks the produced
    file.  The process is started when ``start_recording`` is called and is
    stopped via ``SIGINT`` when ``stop_recording`` is requested.  Recordings
    are always stored on a removable drive that is expected to be automounted
    under ``/media`` or ``/run/media``.  If no such drive is present,
    recording cannot start.
    """

    def __init__(self, mount_roots: Optional[List[str]] = None):
        self.mount_roots = mount_roots or ["/media", "/run/media"]
        self.usb_mount: Optional[Path] = None
        self.output_dir: Optional[Path] = None
        self.log_file: Optional[Path] = None
        self._process: Optional[subprocess.Popen] = None
        self.current_file: Optional[Path] = None
        self.current_started: Optional[datetime] = None
        # Allow overriding the command used to invoke the recorder.
        self.record_cmd = os.getenv("LIVOX_RECORD_CMD", "save_laz")
        self._last_size = 0
        self._last_size_time: Optional[datetime] = None
        self._ensure_storage()

    # ---- internal helpers -------------------------------------------------
    def _find_usb_mount(self) -> Optional[Path]:
        try:
            with open("/proc/mounts") as f:
                for line in f:
                    parts = line.split()
                    if len(parts) < 2:
                        continue
                    mount_point = parts[1]
                    if any(mount_point.startswith(root) for root in self.mount_roots):
                        if os.access(mount_point, os.W_OK):
                            return Path(mount_point)
        except OSError:
            pass
        return None

    def _ensure_storage(self) -> bool:
        mount = self._find_usb_mount()
        if mount != self.usb_mount:
            self.usb_mount = mount
            if mount:
                self.output_dir = mount / "recordings"
                self.output_dir.mkdir(parents=True, exist_ok=True)
                self.log_file = self.output_dir / "recordings.json"
                if not self.log_file.exists():
                    self._write_log([])
            else:
                self.output_dir = None
                self.log_file = None
        return self.output_dir is not None

    def _load_log(self):
        if not self.log_file or not self.log_file.exists():
            return []
        try:
            return json.loads(self.log_file.read_text())
        except json.JSONDecodeError:
            logger.error("Corrupted recordings log detected; resetting")
            self._write_log([])
            return []

    def _write_log(self, data):
        if not self.log_file:
            return
        tmp_path = self.log_file.with_name(self.log_file.name + ".tmp")
        tmp_path.write_text(json.dumps(data, indent=2))
        os.replace(tmp_path, self.log_file)

    def _save_log(self, entry):
        if not self.log_file:
            return
        data = self._load_log()
        data.append(entry)
        self._write_log(data)

    def _probe_lidar(self) -> bool:
        """Invoke the recorder in detection mode to check for a connected LiDAR."""
        try:
            res = subprocess.run(
                [self.record_cmd, "--check"],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                timeout=5,
            )
            return res.returncode == 0
        except (OSError, subprocess.SubprocessError):
            return False

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
            # Check if the previous process has exited
            if self._process.poll() is not None:
                self._process = None
                self.current_file = None
                self.current_started = None
            else:
                # A recording is already running
                return False, "already_active"

        if not self._ensure_storage():
            return False, "no_storage"
        if not self._probe_lidar():
            return False, "no_lidar"
        timestamp = datetime.utcnow().strftime("%Y%m%d_%H%M%S")
        self.current_file = self.output_dir / f"recording_{timestamp}.laz"
        self.current_started = datetime.utcnow()
        self._last_size = 0
        self._last_size_time = None
        cmd = [self.record_cmd, str(self.current_file)]
        try:
            self._process = subprocess.Popen(cmd)
        except OSError:
            # Failed to start external recorder
            self.current_file = None
            self.current_started = None
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
            "started": self.current_started.isoformat() if self.current_started else None,
            "stopped": datetime.utcnow().isoformat()
        }
        self._save_log(entry)
        self._process = None
        self.current_file = None
        self.current_started = None
        self._last_size = 0
        self._last_size_time = None
        return True

    def status(self):
        storage = self._ensure_storage()
        if self._process is not None and self._process.poll() is not None:
            self._process = None
            self.current_file = None
            self.current_started = None
        lidar_detected = self._probe_lidar()
        lidar_streaming = False
        if self._process and self.current_file:
            try:
                size = self.current_file.stat().st_size
                now = datetime.utcnow()
                if size != self._last_size:
                    self._last_size = size
                    self._last_size_time = now
                    lidar_streaming = True
                elif self._last_size_time and (now - self._last_size_time).total_seconds() < 2:
                    lidar_streaming = True
            except OSError:
                pass
        return {
            "recording": self._process is not None,
            "current_file": self.current_file.name if self.current_file else None,
            "started": self.current_started.isoformat() if self.current_started else None,
            "storage_present": storage,
            "lidar_detected": lidar_detected,
            "lidar_streaming": lidar_streaming,
        }

    def list_recordings(self):
        self._ensure_storage()
        return self._load_log()
