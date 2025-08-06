import logging
import logging.handlers
import os
import re
import subprocess
from pathlib import Path
from typing import List, Optional


def _find_usb_mount(mount_roots: List[str]) -> Optional[Path]:
    """Return the first writable ``<root>/usbN`` mount under ``mount_roots``.

    The helper probes the system using ``mount`` and ``lsblk`` and falls back
    to scanning the filesystem directly.  Any detected mount point must be
    writable before it is returned.
    """

    prefixes = [f"{r.rstrip('/')}/usb" for r in mount_roots if r]

    # Probe using the ``mount`` command
    try:
        result = subprocess.run(["mount"], capture_output=True, text=True, check=False)
        for line in result.stdout.splitlines():
            parts = line.split()
            if len(parts) >= 3:
                mp = parts[2]
                if any(mp.startswith(p) for p in prefixes) and os.access(mp, os.W_OK):
                    return Path(mp)
    except OSError:
        pass

    # Fallback to ``lsblk`` which lists mount points per block device
    try:
        result = subprocess.run(
            ["lsblk", "-nr", "-o", "MOUNTPOINT"],
            capture_output=True,
            text=True,
            check=False,
        )
        for mp in result.stdout.splitlines():
            if any(mp.startswith(p) for p in prefixes) and os.access(mp, os.W_OK):
                return Path(mp)
    except OSError:
        pass

    # Final fallback: scan the mount roots directly
    for root in mount_roots:
        base = Path(root.rstrip("/"))
        try:
            candidates = [
                p
                for p in base.glob("usb*")
                if os.path.ismount(p) and os.access(p, os.W_OK)
            ]
        except OSError:
            continue
        if candidates:
            candidates.sort(
                key=lambda p: int(re.search(r"usb(\d+)$", p.name).group(1))
                if re.search(r"usb(\d+)$", p.name)
                else float("inf"),
            )
            return candidates[0]

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
