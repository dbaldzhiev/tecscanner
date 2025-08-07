#!/usr/bin/env bash
# Install and configure Tecscanner on Raspberry Pi OS (Bookworm 64-bit)
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

prompt=true
if [ "$1" = "--noprompt" ]; then
  prompt=false
fi

steps=(
  update_submodules.sh
  configure_swap.sh
  install_packages.sh
  setup_python_env.sh
  setup_service.sh
  build_livox_sdk2.sh
  build_laszip.sh
  compile_save_laz.sh
  setup_usb_mount.sh
  configure_network.sh
)

for step in "${steps[@]}"; do
  "$SCRIPT_DIR/$step"
  if [ "$prompt" = true ]; then
    read -r -p "Continue with next operation? (y/N) " choice
    case "$choice" in
      [Yy]*) ;;
      *) echo "Process halted."; exit 0 ;;
    esac
  fi
done

echo "All operations complete."

