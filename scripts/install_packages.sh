#!/usr/bin/env bash
# Install required system packages.
set -e

packages=(git build-essential cmake python3-venv python3-dev network-manager)
missing_packages=()
for pkg in "${packages[@]}"; do
  if ! dpkg -s "$pkg" >/dev/null 2>&1; then
    missing_packages+=("$pkg")
  fi
done

if [ ${#missing_packages[@]} -gt 0 ]; then
  echo "Installing system packages..."
  sudo apt update
  sudo apt install -y "${missing_packages[@]}"
else
  echo "System packages already installed."
fi

