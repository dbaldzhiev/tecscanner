#!/usr/bin/env bash
# Compile and install the save_laz utility together with its dependencies.
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CODE_DIR="$PROJECT_ROOT/code"

# Fetch submodules in case they are missing
"$PROJECT_ROOT/scripts/update_submodules.sh"

# Ensure third-party libraries are available
"$PROJECT_ROOT/scripts/build_livox_sdk2.sh" || echo "Livox-SDK2 build failed; assuming pre-installed"
"$PROJECT_ROOT/scripts/build_laszip.sh" || {
  echo "LASzip build failed; attempting to install system package";
  sudo apt-get update;
  sudo apt-get install -y liblaszip-dev;
}

# Install nlohmann-json headers if not present
if [ ! -f /usr/include/nlohmann/json.hpp ]; then
  sudo apt-get update
  sudo apt-get install -y nlohmann-json3-dev
fi

echo "Compiling save_laz utility..."
cmake -S "$CODE_DIR" -B "$CODE_DIR/build"
cmake --build "$CODE_DIR/build" --config Release
sudo install -m 0755 "$CODE_DIR/build/save_laz" /usr/local/bin/save_laz

