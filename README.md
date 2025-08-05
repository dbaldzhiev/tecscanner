# Tecscanner

Tecscanner provides a web interface for controlling a Livox MID360 LiDAR and recording frames directly to a USB drive. The project targets Raspberry Pi 4 (64‑bit, Debian Bookworm headless).

## Quick Install

Execute the script to build all native components, set up Python, configure USB automounting and assign a static IP to `eth0`:

```bash
bash scripts/install.sh
```

## Manual Installation

The steps below mirror the script and can be copied individually.

### 1. Clone the repository

```bash
git clone https://github.com/dbaldzhiev/tecscanner.git
cd tecscanner
git submodule update --init --recursive
```

### 2. Expand swap to 2 GB

```bash
sudo sed -i 's/^CONF_SWAPSIZE=.*/CONF_SWAPSIZE=2048/' /etc/dphys-swapfile
sudo systemctl restart dphys-swapfile
```

### 3. Install required packages

```bash
sudo apt update
sudo apt install -y git build-essential cmake python3-venv python3-dev usbmount
```

### 4. Build Livox‑SDK2

```bash
cd 3rd/Livox-SDK2
mkdir -p build && cd build
cmake ..
make -j$(nproc)
sudo make install
cd ../../..
```

### 5. Build LASzip

```bash
cd 3rd/LASzip
mkdir -p build && cd build
cmake ..
make -j$(nproc)
sudo make install
cd ../../..
```

### 6. Compile `save_laz`

```bash
cd save_laz
cmake -S . -B build
cmake --build build --config Release
sudo install -m 0755 build/save_laz /usr/local/bin/save_laz
cd ..
```

### 7. Set up Python and Flask

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install --upgrade pip
pip install Flask
```

### 8. Enable USB auto‑mount

```bash
sudo apt install -y usbmount
```

Drives will appear under `/media/usb0`, which the app monitors.

### 9. Assign static IP to `eth0`

```bash
sudo tee -a /etc/dhcpcd.conf <<'EOF'
interface eth0
static ip_address=192.168.6.1/24
EOF
sudo systemctl restart dhcpcd
```

### 10. Run the web app

```bash
source .venv/bin/activate
flask --app webapp run --host=0.0.0.0
```

## API Endpoints

- `POST /start` – begin recording
- `POST /stop` – end the current recording
- `GET /status` – current status in JSON
- `GET /recordings` – list previous sessions

## License

See [LICENSE](LICENSE) for details.
