# MID360 Remote Controller

This repository provides a lightweight web application for remotely
controlling a Livox MID360 LiDAR. The web interface exposes controls to
start and stop recordings and displays a list of previous recordings.
No physical buttons or switches are required; all interaction happens
through the browser.

## Features
- Start a new LiDAR recording.
- Stop the current recording.
- View status and a history of recorded files.

The example implementation stores recordings in the `recordings/`
directory and logs metadata in `recordings/recordings.json`. Replace the
mocked sections in `recording_manager.py` with calls to the actual Livox
SDK for hardware integration.

## Getting Started
1. Install dependencies:
   ```bash
   pip install -r requirements.txt
   ```
2. Run the development server:
   ```bash
   flask --app webapp run
   ```
3. Open a browser at [http://localhost:5000](http://localhost:5000) to
   control the scanner.

## API Endpoints
- `POST /start` – begin recording.
- `POST /stop` – end the current recording.
- `GET /status` – retrieve JSON describing the current state.
- `GET /recordings` – list metadata for previous recordings.

## License
See [LICENSE](LICENSE) for license information.
