import threading
import time
from pathlib import Path
from datetime import datetime
import json

class RecordingManager:
    """Manage MID360 recordings.

    This implementation simulates interaction with the Livox device. In a
    production setup, replace the mocked sections with real SDK calls that
    stream data to ``self.current_file``.
    """

    def __init__(self, output_dir: str = "recordings"):
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        self._recording_thread = None
        self.current_file = None
        self.log_file = self.output_dir / "recordings.json"
        if not self.log_file.exists():
            self.log_file.write_text("[]")

    # ---- internal helpers -------------------------------------------------
    def _save_log(self, entry):
        data = json.loads(self.log_file.read_text())
        data.append(entry)
        self.log_file.write_text(json.dumps(data, indent=2))

    def _record(self, path: Path):
        with path.open("w") as f:
            while self._recording_thread is not None:
                f.write(f"point {time.time()}\n")
                f.flush()
                time.sleep(0.1)

    # ---- public API -------------------------------------------------------
    def start_recording(self) -> bool:
        if self._recording_thread is not None:
            return False
        timestamp = datetime.utcnow().strftime("%Y%m%d_%H%M%S")
        self.current_file = self.output_dir / f"recording_{timestamp}.laz"
        self._recording_thread = threading.Thread(target=self._record, args=(self.current_file,), daemon=True)
        self._recording_thread.start()
        return True

    def stop_recording(self) -> bool:
        if self._recording_thread is None:
            return False
        thread = self._recording_thread
        self._recording_thread = None
        thread.join()
        entry = {
            "file": self.current_file.name,
            "stopped": datetime.utcnow().isoformat()
        }
        self._save_log(entry)
        self.current_file = None
        return True

    def status(self):
        return {
            "recording": self._recording_thread is not None,
            "current_file": self.current_file.name if self.current_file else None,
        }

    def list_recordings(self):
        return json.loads(self.log_file.read_text())
