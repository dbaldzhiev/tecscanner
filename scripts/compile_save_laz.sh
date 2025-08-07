#!/usr/bin/env bash
# Compile and install the save_laz utility.
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if [ ! -f /usr/local/bin/save_laz ]; then
  echo "Compiling save_laz utility..."
  cmake -S "$PROJECT_ROOT/save_laz" -B "$PROJECT_ROOT/save_laz/build"
  cmake --build "$PROJECT_ROOT/save_laz/build" --config Release
  sudo install -m 0755 "$PROJECT_ROOT/save_laz/build/save_laz" /usr/local/bin/save_laz
else
  echo "save_laz utility already installed."
fi

