from __future__ import annotations

"""Utility helpers for auxiliary recording artefacts.

The functions in this module generate supplemental files for each
recorded frame, mirroring the behaviour of the utilities in the
``mandeye_controller`` project.  They are deliberately lightweight and
make a best effort to gather data from the local system; missing
hardware or optional tools are tolerated gracefully.
"""

from pathlib import Path
import json
import subprocess
import time
from typing import Iterable


def _write_lines(path: Path, lines: Iterable[str]) -> None:
    """Write ``lines`` to ``path`` ending with a newline."""
    text = "\n".join(lines)
    if text:
        text += "\n"
    path.write_text(text)


def write_lidar_sn(path: Path) -> None:
    """Dump connected LiDAR serial numbers to ``path``.

    The function attempts to invoke the ``save_laz`` binary with a
    hypothetical ``--sn`` flag to query connected devices.  If the call
    fails or produces no output, an empty file is written.
    """
    try:
        result = subprocess.run(
            ["save_laz", "--sn"], capture_output=True, text=True, timeout=5, check=True
        )
        lines = [line.strip() for line in result.stdout.splitlines() if line.strip()]
    except (OSError, subprocess.SubprocessError):
        lines = []
    _write_lines(path, lines)


def write_status(path: Path, **extra) -> None:
    """Write a JSON blob with a runtime status snapshot.

    Any keyword arguments supplied in ``extra`` are merged into the
    output dictionary.  The snapshot always includes a Unix timestamp.
    """
    data = {"timestamp": time.time(), **extra}
    path.write_text(json.dumps(data, indent=2))


def write_gnss(processed_path: Path, raw_path: Path) -> None:
    """Capture processed and raw GNSS strings.

    ``gpspipe`` is used if available; otherwise empty files are created.
    """
    try:
        proc = subprocess.run(
            ["gpspipe", "-w", "-n", "1"], capture_output=True, text=True, timeout=5, check=True
        )
        processed_path.write_text(proc.stdout)
    except (OSError, subprocess.SubprocessError):
        processed_path.write_text("")
    try:
        raw = subprocess.run(
            ["gpspipe", "-r", "-n", "1"], capture_output=True, text=True, timeout=5, check=True
        )
        raw_path.write_text(raw.stdout)
    except (OSError, subprocess.SubprocessError):
        raw_path.write_text("")
