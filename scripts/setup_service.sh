#!/usr/bin/env bash
# Create and enable a systemd service for the Tecscanner Flask app.
set -e

SERVICE_NAME=tecscanner
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
USER_NAME=${SUDO_USER:-$(whoami)}

PYTHON_BIN="$PROJECT_ROOT/.venv/bin/python"
if [ ! -f "$PYTHON_BIN" ]; then
  PYTHON_BIN=$(command -v python3)
fi

sudo tee /etc/systemd/system/${SERVICE_NAME}.service > /dev/null <<SERVICE
[Unit]
Description=Tecscanner Flask Service
After=network.target

[Service]
Type=simple
User=${USER_NAME}
WorkingDirectory=${PROJECT_ROOT}
Environment=FLASK_APP=webapp
ExecStart=${PYTHON_BIN} -m flask run --host=0.0.0.0 --port=5000
Restart=on-failure

[Install]
WantedBy=multi-user.target
SERVICE

sudo systemctl daemon-reload
sudo systemctl enable ${SERVICE_NAME}
sudo systemctl start ${SERVICE_NAME}

echo "Service ${SERVICE_NAME} installed and started."
