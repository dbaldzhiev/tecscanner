#!/usr/bin/env bash
# Create Python virtual environment and install dependencies.
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if [ ! -d "$PROJECT_ROOT/.venv" ]; then
  echo "Creating Python virtual environment..."
  python3 -m venv "$PROJECT_ROOT/.venv"
fi

# shellcheck source=/dev/null
source "$PROJECT_ROOT/.venv/bin/activate"
pip install --upgrade pip
if ! pip show Flask >/dev/null 2>&1; then
  pip install Flask
fi

