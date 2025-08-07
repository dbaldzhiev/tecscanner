#!/usr/bin/env bash
# Build the LASzip library.
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if [ ! -f "$PROJECT_ROOT/3rd/LASzip/CMakeCache.txt" ]; then
  echo "Building LASzip..."
  cd "$PROJECT_ROOT/3rd/LASzip" || exit 1
  cmake -DCMAKE_BUILD_TYPE=Release CMakeLists.txt
  cmake --build .
else
  echo "LASzip already built."
fi

