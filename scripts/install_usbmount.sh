#!/usr/bin/env bash
# Install USB auto-mount helper and udev rules.
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

packages=(exfatprogs ntfs-3g)
missing_packages=()
for pkg in "${packages[@]}"; do
  if ! dpkg -s "$pkg" >/dev/null 2>&1; then
    missing_packages+=("$pkg")
  fi
done
if [ ${#missing_packages[@]} -gt 0 ]; then
  echo "Installing USB mount packages..."
  sudo apt update
  sudo apt install -y "${missing_packages[@]}"
else
  echo "USB mount packages already installed."
fi

if [ ! -f /usr/local/sbin/usb-automount.sh ] || [ ! -f /etc/udev/rules.d/99-usb-automount.rules ]; then
  echo "Installing USB automount udev rules..."
  sudo install -m 0755 "$PROJECT_ROOT/scripts/usb-automount.sh" /usr/local/sbin/usb-automount.sh
  sudo install -m 0644 "$PROJECT_ROOT/scripts/99-usb-automount.rules" /etc/udev/rules.d/99-usb-automount.rules
  sudo udevadm control --reload-rules
else
  echo "USB automount already configured."
fi
