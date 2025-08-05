#!/usr/bin/env bash
# Setup Tecscanner on Raspberry Pi OS (Bookworm 64-bit)
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "Ensuring git submodules are initialized..."
cd "$PROJECT_ROOT"
git submodule update --init --recursive

echo "Expanding swap to 2GB..."
if [ -f /etc/dphys-swapfile ]; then
  sudo sed -i 's/^CONF_SWAPSIZE=.*/CONF_SWAPSIZE=2048/' /etc/dphys-swapfile
  sudo systemctl restart dphys-swapfile || true
fi

echo "Installing system packages..."
sudo apt update
sudo apt install -y git build-essential cmake python3-venv python3-dev usbmount network-manager

echo "Building Livox-SDK2..."
cmake -S "$PROJECT_ROOT/3rd/Livox-SDK2" -B "$PROJECT_ROOT/3rd/Livox-SDK2/build"
cmake --build "$PROJECT_ROOT/3rd/Livox-SDK2/build" --config Release
sudo cmake --install "$PROJECT_ROOT/3rd/Livox-SDK2/build"

echo "Building LASzip..."
cmake -S "$PROJECT_ROOT/3rd/LASzip" -B "$PROJECT_ROOT/3rd/LASzip/build"
cmake --build "$PROJECT_ROOT/3rd/LASzip/build" --config Release
sudo cmake --install "$PROJECT_ROOT/3rd/LASzip/build"

echo "Compiling save_laz utility..."
cmake -S "$PROJECT_ROOT/save_laz" -B "$PROJECT_ROOT/save_laz/build"
cmake --build "$PROJECT_ROOT/save_laz/build" --config Release
sudo install -m 0755 "$PROJECT_ROOT/save_laz/build/save_laz" /usr/local/bin/save_laz

if [ ! -d "$PROJECT_ROOT/.venv" ]; then
  echo "Creating Python virtual environment..."
  python3 -m venv "$PROJECT_ROOT/.venv"
fi
source "$PROJECT_ROOT/.venv/bin/activate"
pip install --upgrade pip
pip install Flask

echo "Configuring static IP 192.168.6.1 on eth0 via NetworkManager..."
sudo nmcli con mod "Wired connection 1" ipv4.method manual ipv4.addresses 192.168.6.1/24
sudo nmcli con mod "Wired connection 1" ipv4.gateway ""
sudo nmcli con mod "Wired connection 1" ipv4.dns ""
sudo nmcli con up "Wired connection 1"

echo "Installation complete."
