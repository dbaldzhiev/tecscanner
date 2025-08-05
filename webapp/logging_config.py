import logging
import logging.handlers
import os
from pathlib import Path
from typing import Optional, List


def _find_usb_mount(mount_roots: List[str]) -> Optional[Path]:
    """Return the first writable mount point under ``mount_roots``."""
    try:
        with open("/proc/mounts") as f:
            for line in f:
                parts = line.split()
                if len(parts) < 2:
                    continue
                mount_point = parts[1]
                if any(mount_point.startswith(root) for root in mount_roots):
                    if os.access(mount_point, os.W_OK):
                        return Path(mount_point)
    except OSError:
        pass
    return None


def configure_logging() -> Optional[Path]:
    """Configure logging to console and a file on the USB drive if available.

    Returns the path to the log file if one was created, otherwise ``None``.
    """
    root = logging.getLogger()
    if root.handlers:
        # Logging already configured
        for h in root.handlers:
            if isinstance(h, logging.FileHandler):
                try:
                    return Path(h.baseFilename)
                except Exception:
                    pass
        return None

    level = os.getenv("LOG_LEVEL", "INFO").upper()
    root.setLevel(level)

    fmt = logging.Formatter("%(asctime)s %(levelname)s [%(name)s] %(message)s")

    stream = logging.StreamHandler()
    stream.setFormatter(fmt)
    root.addHandler(stream)

    roots = os.getenv("LIVOX_MOUNT_ROOTS", "/media:/run/media").split(os.pathsep)
    mount = _find_usb_mount(roots)
    if not mount:
        root.warning("No writable USB storage found; file logging disabled")
        return None

    log_dir = mount / "logs"
    try:
        log_dir.mkdir(parents=True, exist_ok=True)
    except OSError:
        root.warning("Unable to create log directory %s", log_dir)
        return None

    log_file = log_dir / "tecscanner.log"
    file_handler = logging.handlers.RotatingFileHandler(
        log_file, maxBytes=5 * 1024 * 1024, backupCount=3
    )
    file_handler.setFormatter(fmt)
    root.addHandler(file_handler)
    root.info("Logging initialised; writing to %s", log_file)
    return log_file
