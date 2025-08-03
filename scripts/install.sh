#!/usr/bin/env bash
# Install system and Python dependencies for Tecscanner
# and build required native components.
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "Installing system packages..."
sudo apt-get update
sudo apt-get install -y build-essential cmake python3-dev python3-venv

echo "Compiling Livox-SDK2..."
cd "$PROJECT_ROOT/3rd/Livox-SDK2"
mkdir -p build && cd build
cmake ..
make -j"$(nproc)"
sudo make install

echo "Building save_laz utility..."
cd "$PROJECT_ROOT/save_laz"
cmake -S . -B build
cmake --build build --config Release
sudo cp build/save_laz /usr/local/bin/save_laz

cd "$PROJECT_ROOT"

if [ ! -d "$PROJECT_ROOT/.venv" ]; then
  echo "Creating Python virtual environment..."
  python3 -m venv .venv
fi

source .venv/bin/activate
pip install --upgrade pip
pip install -r requirements.txt

echo "Installation complete."
