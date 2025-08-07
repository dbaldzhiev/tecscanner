#!/usr/bin/env bash
# Ensure git submodules are initialized.
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "Ensuring git submodules are initialized..."
cd "$PROJECT_ROOT"
git submodule update --init --recursive

