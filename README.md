# Remote Mandeye Controller for HDMapping

This repository provides a lightweight web application for remotely
controlling a Livox MID360 LiDAR. It is inspired by and compatible with
[mandeye_controller](https://github.com/JanuszBedkowski/mandeye_controller)
and [HDMapping](https://github.com/MapsHD/HDMapping). The web interface
exposes controls to start and stop recordings and displays a list of
previous recordings. No physical buttons or switches are required; all
interaction happens through the browser.

## Features
- Start a new LiDAR recording.
- Stop the current recording.
- View status and a history of recorded files.
- Recordings are saved to an attached USB drive and the UI warns if no drive is present.
- Live status warnings when no LiDAR is detected or no data is streaming.

Recordings are written to a USB flash drive that is expected to be
automounted by the operating system. Metadata is tracked in
`recordings/recordings.json` on that drive. The
`RecordingManager` launches the Livox SDK recorder (similar to the
`save_laz` utility from
[`mandeye_controller`](https://github.com/JanuszBedkowski/mandeye_controller))
to capture real MID360 data. For every frame the recorder writes both a
compressed `.laz` file and a companion `.csv` export in the session
directory.

## Installation
A convenience script is available to install dependencies and build native
components automatically:

```bash
bash scripts/install.sh
```

Alternatively, follow the manual steps below.

1. Install required system packages:
   ```bash
   sudo apt-get install build-essential cmake python3-dev python3-venv
   ```
2. Install LASzip (required by `save_laz`):
   ```bash
   git clone https://github.com/LAStools/LAStools.git
   cd LAStools
   mkdir build
   cd build
   cmake -DCMAKE_BUILD_TYPE=Release ..
   cmake --build .
   sudo mkdir -p /usr/local/bin
   for f in ../bin64/*; do sudo ln -sf "$(readlink -e "$f")" /usr/local/bin/$(basename "${f%64}"); done
   cd ../../
   ```
3. Clone the repository:
   ```bash
   git clone https://github.com/<your-username>/tecscanner.git
   cd tecscanner
   ```
4. (Optional) create and activate a virtual environment:
   ```bash
   python3 -m venv .venv
   source .venv/bin/activate
   ```
5. Install Python dependencies:
   ```bash
   pip install -r requirements.txt
   ```
6. Build the Livox recording utility:
   - The repository includes a minimal `save_laz` program under
     `save_laz/` which streams MID360 data directly to `.laz` files (and
     simultaneously produces `.csv` versions) using
     [LASzip](https://laszip.org/).
   - Build it with CMake:
     ```bash
     cmake -S save_laz -B save_laz/build
     cmake --build save_laz/build
     ```
   - Make sure the resulting `save_laz` binary is on your `PATH` or set the
     `LIVOX_RECORD_CMD` environment variable to point to it so the web
     application can invoke it.
7. Configure the Livox MID360:
   - Connect the LiDAR and host via Ethernet.
   - Assign the host interface a static IP such as `192.168.1.5`.
   - Ensure the MID360 is reachable on the same subnet (the factory default is
     typically `192.168.1.10`).
   - Update the provided `mid360_config.json` with the host IP and desired
     ports (defaults mirror the Livox SDK samples using ports 56100–56501).
     Place this file next to the `save_laz` binary or set `LIVOX_SDK_CONFIG`
     to its location.
8. (Optional) specify where to look for removable storage:
   - By default the application searches `/media` and `/run/media` for a mounted
     USB drive. Set the `LIVOX_MOUNT_ROOTS` environment variable to a
     colon-separated list of paths to check additional locations.
     ```bash
     export LIVOX_MOUNT_ROOTS=/media:/mnt/usb
     ```

## Recording Log Retention
The `recordings.json` file on the USB drive stores metadata for recent
recordings. By default only the most recent 100 entries are retained. Set the
`RECORDINGS_LOG_LIMIT` environment variable to change this limit. If
`RECORDINGS_LOG_ARCHIVE` is set to a truthy value (e.g. `1`), entries beyond
the limit are appended to `recordings_archive.json` in the same directory.

## Running
1. Start the Livox recorder and web server:
   ```bash
   flask --app webapp run --host=0.0.0.0
   ```
2. Open a browser at [http://localhost:5000](http://localhost:5000) or use
   the host's IP address if accessing from another machine to control the
   scanner.

To have the web application start automatically on boot, run
`scripts/setup_service.sh`. The script creates and enables a `systemd`
service that launches the Flask application using this repository's virtual
environment.

## API Endpoints
- `POST /start` – begin recording.
- `POST /stop` – end the current recording.
- `GET /status` – retrieve JSON describing the current state.
- `GET /recordings` – list metadata for previous recordings.

## License
See [LICENSE](LICENSE) for license information.
