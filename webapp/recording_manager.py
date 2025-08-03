"""Utilities for managing Livox recordings.

The original implementation in this repository merely simulated a LiDAR
device by periodically writing mock "point" entries to a file.  This module
invokes the Livox SDK via an external recording command to capture real
frames.  Each frame is written to an individual ``.laz`` file inside a
timestamped session directory on the removable storage, accompanied by a
``.csv`` export of the same data.

The path to the Livox recorder executable can be configured via the
``LIVOX_RECORD_CMD`` environment variable.  It should point to a command that
accepts the desired output filename as its last argument and exits after
capturing a frame.
"""

import json
import os
import subprocess
import threading
import time
from typing import Optional, List
from datetime import datetime
from pathlib import Path
import logging
import shutil

logger = logging.getLogger(__name__)

class RecordingManager:
    """Manage MID360 recordings by delegating to the Livox SDK.

    The manager repeatedly invokes an external recorder command (typically the
    ``save_laz`` utility from ``mandeye_controller``) to capture frames.  Each
    captured frame is stored as ``frame_<n>.laz`` (with a matching
    ``frame_<n>.csv``) inside a dedicated session directory.  Recordings are
    always stored on a removable drive that is expected to be automounted under
    ``/media`` or ``/run/media``.  If no such drive is present, recording cannot
    start.
    """

    def __init__(self, mount_roots: Optional[List[str]] = None):
        if mount_roots is not None:
            self.mount_roots = mount_roots
        else:
            env_roots = os.getenv("LIVOX_MOUNT_ROOTS")
            if env_roots:
                self.mount_roots = [r for r in env_roots.split(os.pathsep) if r]
            else:
                self.mount_roots = ["/media", "/run/media"]
        self.usb_mount: Optional[Path] = None
        self.output_dir: Optional[Path] = None
        self.log_file: Optional[Path] = None
        self.archive_file: Optional[Path] = None
        self._thread: Optional[threading.Thread] = None
        self._stop_event: Optional[threading.Event] = None
        self.current_dir: Optional[Path] = None
        self.current_file: Optional[Path] = None
        self.current_started: Optional[datetime] = None
        self.frame_counter: int = 0
        # Allow overriding the command used to invoke the recorder.
        cmd = os.getenv("LIVOX_RECORD_CMD", "save_laz")
        resolved = shutil.which(cmd)
        if resolved:
            self.record_cmd = resolved
            self._recorder_available = True
        else:
            logger.error("Recorder command '%s' not found; recordings disabled", cmd)
            self.record_cmd = None
            self._recorder_available = False
        conv = os.getenv("LIVOX_CONVERT_CMD")
        self.convert_cmd = shutil.which(conv) if conv else None
        self._last_size = 0
        self._last_size_time: Optional[datetime] = None
        self._lock = threading.Lock()
        self.max_log_entries = int(os.getenv("RECORDINGS_LOG_LIMIT", "100"))
        archive_env = os.getenv("RECORDINGS_LOG_ARCHIVE", "").lower()
        self.archive_enabled = archive_env in ("1", "true", "yes")
        self._log_error = False
        self._archive_error = False
        # Cache LiDAR detection result and refresh periodically
        self._lidar_detected = self._probe_lidar()
        self._probe_interval = float(os.getenv("LIDAR_PROBE_INTERVAL", "5"))
        self._detector_stop = threading.Event()
        self._detector_thread = threading.Thread(
            target=self._detection_loop, daemon=True
        )
        self._detector_thread.start()
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
                self.archive_file = self.output_dir / "recordings_archive.json"
                if not self.log_file.exists():
                    self._write_log([])
            else:
                self.output_dir = None
                self.log_file = None
                self.archive_file = None
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
        try:
            with tmp_path.open("w") as f:
                f.write(json.dumps(data, indent=2))
                f.flush()
                os.fsync(f.fileno())
            os.replace(tmp_path, self.log_file)
            self._log_error = False
        except OSError as e:
            logger.warning("Failed to write recordings log: %s", e)
            self._log_error = True

    def _save_log(self, entry):
        if not self.log_file:
            return
        data = self._load_log()
        data.append(entry)
        overflow = []
        if self.max_log_entries and len(data) > self.max_log_entries:
            overflow = data[:-self.max_log_entries]
            data = data[-self.max_log_entries:]
        if overflow and self.archive_enabled:
            self._archive_log(overflow)
        self._write_log(data)

    def _archive_log(self, entries):
        if not self.archive_file or not entries:
            return
        try:
            existing = (
                json.loads(self.archive_file.read_text())
                if self.archive_file.exists()
                else []
            )
        except json.JSONDecodeError:
            existing = []
        existing.extend(entries)
        tmp_path = self.archive_file.with_name(self.archive_file.name + ".tmp")
        try:
            with tmp_path.open("w") as f:
                f.write(json.dumps(existing, indent=2))
                f.flush()
                os.fsync(f.fileno())
            os.replace(tmp_path, self.archive_file)
            self._archive_error = False
        except OSError as e:
            logger.warning("Failed to archive recordings log: %s", e)
            self._archive_error = True

    def _probe_lidar(self) -> bool:
        """Invoke the recorder in detection mode to check for a connected LiDAR."""
        if not self.record_cmd:
            return False
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

    def _detection_loop(self) -> None:
        """Background thread to refresh cached LiDAR detection results."""
        while not self._detector_stop.is_set():
            detected = self._probe_lidar()
            with self._lock:
                self._lidar_detected = detected
            # Allow early exit during the wait period
            self._detector_stop.wait(self._probe_interval)

    def _save_frame(self, path: Path) -> bool:
        """Invoke the recorder to capture a single frame to ``path``."""
        if not self.record_cmd:
            return False
        try:
            subprocess.run([self.record_cmd, str(path)], check=True)
            return True
        except (OSError, subprocess.SubprocessError) as e:
            logger.warning("Failed to save frame %s: %s", path, e)
            return False

    def _convert_to_csv(self, laz: Path, csv: Path) -> bool:
        """Convert ``laz`` to ``csv`` using an external command if available."""
        if self.convert_cmd:
            try:
                subprocess.run([self.convert_cmd, str(laz), str(csv)], check=True)
                return True
            except (OSError, subprocess.SubprocessError) as e:
                logger.warning("Failed to convert %s to CSV: %s", laz, e)
                return False
        return csv.exists()

    def _finalize_recording(self, success: bool, error: Optional[str] = None) -> None:
        """Join the worker thread, reset state and log the result."""
        thread = self._thread
        if thread and thread is not threading.current_thread():
            thread.join()
        with self._lock:
            entry = {
                "folder": self.current_dir.name if self.current_dir else None,
                "frames": self.frame_counter,
                "started": self.current_started.isoformat()
                if self.current_started
                else None,
                "stopped": datetime.utcnow().isoformat(),
            }
            if not success:
                entry["error"] = error or "save_failed"
            self._save_log(entry)
            self._thread = None
            self._stop_event = None
            self.current_dir = None
            self.current_file = None
            self.current_started = None
            self.frame_counter = 0
            self._last_size = 0
            self._last_size_time = None

    def _record_loop(self) -> None:
        """Background loop that saves frames until stopped."""
        frame_idx = 0
        failures = 0
        max_failures = 3
        while self._stop_event and not self._stop_event.is_set():
            if not self.current_dir:
                break
            path = self.current_dir / f"frame_{frame_idx:06d}.laz"
            csv_path = path.with_suffix(".csv")
            if not self._save_frame(path):
                failures += 1
                logger.error("Failed to save frame %s", path)
                if failures <= max_failures:
                    time.sleep(1)
                    continue
                self._finalize_recording(False, "save_failed")
                return
            failures = 0
            if not csv_path.exists():
                self._convert_to_csv(path, csv_path)
            try:
                size = path.stat().st_size
            except OSError:
                size = 0
            now = datetime.utcnow()
            with self._lock:
                self.current_file = path
                self.frame_counter = frame_idx + 1
                self._last_size = size
                self._last_size_time = now
            frame_idx += 1

    # ---- public API -------------------------------------------------------
    def start_recording(self) -> tuple[bool, Optional[str]]:
        """Start a Livox recording.

        Returns a tuple ``(started, error)`` where ``started`` indicates
        whether the recording thread was launched and ``error`` is ``None`` on
        success or a string identifying the failure.  Possible error codes are
        ``"already_active"`` when a recording is in progress, ``"no_recorder"``
        when the recorder command is missing, and ``"spawn_failed"`` when the
        recording loop cannot be created.
        """

        with self._lock:
            if not self._recorder_available:
                return False, "no_recorder"
            if self._thread and self._thread.is_alive():
                return False, "already_active"
            if not self._ensure_storage():
                return False, "no_storage"
            cached_detected = self._lidar_detected

        # Probe for the LiDAR outside the lock to avoid blocking other calls
        if not cached_detected:
            cached_detected = self._probe_lidar()

        with self._lock:
            self._lidar_detected = cached_detected
            if not cached_detected:
                return False, "no_lidar"
            if self._thread and self._thread.is_alive():
                return False, "already_active"
            if not self._ensure_storage():
                return False, "no_storage"

            timestamp = datetime.utcnow().strftime("%Y%m%d_%H%M%S")
            self.current_dir = self.output_dir / f"session_{timestamp}"
            self.current_dir.mkdir(parents=True, exist_ok=True)
            self.current_started = datetime.utcnow()
            self.current_file = None
            self.frame_counter = 0
            self._last_size = 0
            self._last_size_time = None
            self._stop_event = threading.Event()
            self._thread = threading.Thread(target=self._record_loop, daemon=True)
            try:
                self._thread.start()
            except RuntimeError:
                self._thread = None
                self._stop_event = None
                self.current_dir = None
                self.current_started = None
                return False, "spawn_failed"
            return True, None

    def stop_recording(self) -> bool:
        """Stop the Livox recording and log the result."""
        with self._lock:
            if not self._thread or not self._thread.is_alive():
                return False
            if self._stop_event:
                self._stop_event.set()
        self._finalize_recording(True)
        return True

    def status(self):
        with self._lock:
            storage = self._ensure_storage()
            if self._thread and not self._thread.is_alive():
                self._thread = None
                self._stop_event = None
                self.current_dir = None
                self.current_file = None
                self.current_started = None
                self.frame_counter = 0
            lidar_detected = self._lidar_detected
            lidar_streaming = False
            current_size = None
            if self.current_file:
                try:
                    current_size = self.current_file.stat().st_size
                except OSError:
                    current_size = None
            if self._last_size_time:
                now = datetime.utcnow()
                if (now - self._last_size_time).total_seconds() < 2:
                    lidar_streaming = True
        return {
            "recording": self._thread is not None,
            "current_file": self.current_file.name if self.current_file else None,
            "current_session": self.current_dir.name if self.current_dir else None,
            "started": self.current_started.isoformat() if self.current_started else None,
            "frames_recorded": self.frame_counter,
            "current_size": current_size,
            "storage_present": storage,
            "lidar_detected": lidar_detected,
            "lidar_streaming": lidar_streaming,
            "log_error": self._log_error,
            "archive_error": self._archive_error,
        }

    def list_recordings(self):
        self._ensure_storage()
        return self._load_log()

    def close(self) -> None:
        """Shut down background threads and clean up resources."""
        # Stop an active recording if one is running
        try:
            self.stop_recording()
        except Exception:
            pass
        if self._detector_thread and self._detector_thread.is_alive():
            self._detector_stop.set()
            self._detector_thread.join()
            self._detector_thread = None

    def __del__(self):
        try:
            self.close()
        except Exception:
            pass
