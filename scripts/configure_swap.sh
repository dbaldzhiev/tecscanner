#!/usr/bin/env bash
# Expand swap space to 2GB if needed.
set -e

if [ -f /etc/dphys-swapfile ]; then
  CURRENT_SWAP=$(grep '^CONF_SWAPSIZE=' /etc/dphys-swapfile | cut -d '=' -f2)
  if [ "$CURRENT_SWAP" != "2048" ]; then
    echo "Expanding swap to 2GB..."
    sudo sed -i 's/^CONF_SWAPSIZE=.*/CONF_SWAPSIZE=2048/' /etc/dphys-swapfile
    sudo systemctl restart dphys-swapfile || true
  else
    echo "Swap already set to 2GB."
  fi
fi

