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
import platform
import shutil
import subprocess
import re
from typing import Iterable


def _write_lines(path: Path, lines: Iterable[str]) -> None:
    """Write ``lines`` to ``path`` ending with a newline."""
    text = "\n".join(lines)
    if text:
        text += "\n"
    path.write_text(text)


def write_lidar_sn(path: Path) -> None:
    """Dump connected LiDAR IDs and serial numbers to ``path``.

    Each line in the output file follows the format ``"id SN"`` where
    ``id`` is an integer starting from ``0`` and ``SN`` is the device
    serial number.  If the ``save_laz --sn`` command already returns
    such pairs they are preserved; otherwise the output is enumerated.
    Missing binaries or errors result in an empty file.
    """
    try:
        result = subprocess.run(
            ["save_laz", "--sn"], capture_output=True, text=True, timeout=5, check=True
        )
        raw_lines = [line.strip() for line in result.stdout.splitlines() if line.strip()]
        lines: list[str] = []
        for entry in raw_lines:
            m = re.search(r"(\d+)\s+(\S+)", entry)
            if m:
                lines.append(f"{m.group(1)} {m.group(2)}")
        # If nothing matched but output exists, fall back to enumerating
        if not lines:
            for idx, entry in enumerate(raw_lines):
                tokens = entry.split()
                if tokens:
                    lines.append(f"{idx} {tokens[-1]}")
    except (OSError, subprocess.SubprocessError):
        lines = []
    _write_lines(path, lines)


def _run_cmd(cmd: list[str]) -> str | None:
    """Return stdout from ``cmd`` or ``None`` on failure."""
    try:
        return subprocess.check_output(cmd, text=True).strip()
    except (OSError, subprocess.SubprocessError):
        return None


def write_status(path: Path, **extra) -> None:
    """Write a JSON blob with a runtime status snapshot.

    The resulting structure mirrors ``produceReport`` from the reference
    repository and includes the fields ``name``, ``hash``, ``version``,
    ``hardware``, ``arch``, ``state``, ``livox``, ``gpio``,
    ``fs_benchmark``, ``fs``, ``gnss`` and ``lastLazStatus``.  Any extra
    keyword arguments are merged into the top level of the JSON.
    """
    data = {
        "name": platform.node(),
        "hash": _run_cmd(["git", "rev-parse", "HEAD"]),
        "version": _run_cmd(["git", "describe", "--tags", "--always"]),
        "hardware": platform.machine(),
        "arch": platform.architecture()[0],
        "state": extra.pop("state", None),
        "livox": extra.pop("livox", {}),
        "gpio": extra.pop("gpio", {}),
        "fs_benchmark": extra.pop("fs_benchmark", {}),
        "fs": extra.pop("fs", {}),
        "gnss": extra.pop("gnss", {}),
        "lastLazStatus": extra.pop("lastLazStatus", None),
    }
    # Provide a simple filesystem usage snapshot if none supplied
    if not data["fs"]:
        try:
            usage = shutil.disk_usage("/")
            data["fs"] = {"free": usage.free, "total": usage.total}
        except OSError:
            data["fs"] = {}
    data.update(extra)
    path.write_text(json.dumps(data, indent=2))


def write_gnss(processed_path: Path, raw_path: Path) -> None:
    """Capture processed and raw GNSS strings.

    Multiple lines are recorded for both processed (``.gnss``) and raw
    (``.nmea``) output.  ``gpspipe`` is used if available; otherwise
    empty files are created.
    """
    count = "5"  # number of sentences to capture
    try:
        proc = subprocess.run(
            ["gpspipe", "-w", "-n", count],
            capture_output=True,
            text=True,
            timeout=10,
            check=True,
        )
        processed_path.write_text(proc.stdout)
    except (OSError, subprocess.SubprocessError):
        processed_path.write_text("")
    try:
        raw = subprocess.run(
            ["gpspipe", "-r", "-n", count],
            capture_output=True,
            text=True,
            timeout=10,
            check=True,
        )
        raw_path.write_text(raw.stdout)
    except (OSError, subprocess.SubprocessError):
        raw_path.write_text("")
