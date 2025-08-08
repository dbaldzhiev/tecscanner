#!/usr/bin/env bash
# Compile and install the save_laz utility together with its dependencies.
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CODE_DIR="$PROJECT_ROOT/save_laz"

# Ensure third-party libraries are available
"$PROJECT_ROOT/scripts/build_livox_sdk2.sh"
"$PROJECT_ROOT/scripts/build_laszip.sh"

echo "Compiling save_laz utility..."
cmake -S "$CODE_DIR" -B "$CODE_DIR/build"
cmake --build "$CODE_DIR/build" --config Release
sudo install -m 0755 "$CODE_DIR/build/save_laz" /usr/local/bin/save_laz

