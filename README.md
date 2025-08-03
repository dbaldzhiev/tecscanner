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
directory and logs metadata in `recordings/recordings.json`. Replace the
mocked sections in `recording_manager.py` with calls to the actual Livox
SDK for hardware integration.

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

## Running
1. Start the development server:
   ```bash
   flask --app webapp run
   ```
2. Open a browser at [http://localhost:5000](http://localhost:5000) to
   control the scanner.

## API Endpoints
- `POST /start` – begin recording.
- `POST /stop` – end the current recording.
- `GET /status` – retrieve JSON describing the current state.
- `GET /recordings` – list metadata for previous recordings.

## License
See [LICENSE](LICENSE) for license information.
