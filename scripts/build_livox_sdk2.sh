#!/usr/bin/env bash
# Build the Livox-SDK2 library.
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if [ ! -f "$PROJECT_ROOT/3rd/Livox-SDK2/build/CMakeCache.txt" ]; then
  echo "Building Livox-SDK2..."
  cmake -S "$PROJECT_ROOT/3rd/Livox-SDK2" -B "$PROJECT_ROOT/3rd/Livox-SDK2/build"
  cmake --build "$PROJECT_ROOT/3rd/Livox-SDK2/build" --config Release
  sudo cmake --install "$PROJECT_ROOT/3rd/Livox-SDK2/build"
else
  echo "Livox-SDK2 already built."
fi

