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

The example implementation stores recordings in the `recordings/`
directory and logs metadata in `recordings/recordings.json`. The
`RecordingManager` now launches the Livox SDK recorder (similar to the
`save_laz` utility from
[`mandeye_controller`](https://github.com/JanuszBedkowski/mandeye_controller))
to capture real MID360 data.

## Installation
1. Clone the repository:
   ```bash
   git clone https://github.com/<your-username>/tecscanner.git
   cd tecscanner
   ```
2. (Optional) create and activate a virtual environment:
   ```bash
   python3 -m venv .venv
   source .venv/bin/activate
   ```
3. Install Python dependencies:
   ```bash
   pip install -r requirements.txt
   ```
4. Install the Livox drivers and tooling:
   - Download and build the [Livox SDK](https://github.com/Livox-SDK/Livox-SDK)
     following the instructions in that repository.
   - Build the `mandeye_controller` project to obtain the `save_laz` binary
     which streams LiDAR data to `.laz` files.
   - Ensure the resulting executable is on your `PATH` or set the
     `LIVOX_RECORD_CMD` environment variable to its location so the web
     application can invoke it.

## Running
1. Start the Livox recorder and web server:
   ```bash
   flask --app webapp run --host=0.0.0.0
   ```
2. Open a browser at [http://localhost:5000](http://localhost:5000) or use
   the host's IP address if accessing from another machine to control the
   scanner.

## API Endpoints
- `POST /start` – begin recording.
- `POST /stop` – end the current recording.
- `GET /status` – retrieve JSON describing the current state.
- `GET /recordings` – list metadata for previous recordings.
- `GET /logs/<log_file>` – fetch recorder output for troubleshooting.

## License
See [LICENSE](LICENSE) for license information.
