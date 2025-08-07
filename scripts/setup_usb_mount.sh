#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

if [ ! -f /usr/local/sbin/usb-automount.sh ] || [ ! -f /etc/udev/rules.d/99-usb-automount.rules ]; then
  echo "Installing USB auto-mount support..."
  "$PROJECT_ROOT/scripts/install_usbmount.sh"
else
  echo "USB auto-mount support already installed."
fi
