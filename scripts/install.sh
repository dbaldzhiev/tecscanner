#!/usr/bin/env bash
# Setup Tecscanner on Raspberry Pi OS (Bookworm 64-bit)
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "Ensuring git submodules are initialized..."
cd "$PROJECT_ROOT"
git submodule update --init --recursive

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

if [ ! -f "$PROJECT_ROOT/3rd/Livox-SDK2/build/CMakeCache.txt" ]; then
  echo "Building Livox-SDK2..."
  cmake -S "$PROJECT_ROOT/3rd/Livox-SDK2" -B "$PROJECT_ROOT/3rd/Livox-SDK2/build"
  cmake --build "$PROJECT_ROOT/3rd/Livox-SDK2/build" --config Release
  sudo cmake --install "$PROJECT_ROOT/3rd/Livox-SDK2/build"
else
  echo "Livox-SDK2 already built."
fi

if [ ! -f "$PROJECT_ROOT/3rd/LASzip/build/CMakeCache.txt" ]; then
  echo "Building LASzip..."
  cmake -S "$PROJECT_ROOT/3rd/LASzip" -B "$PROJECT_ROOT/3rd/LASzip/build"
  cmake --build "$PROJECT_ROOT/3rd/LASzip/build" --config Release
  sudo cmake --install "$PROJECT_ROOT/3rd/LASzip/build"
else
  echo "LASzip already built."
fi

if [ ! -f /usr/local/bin/save_laz ]; then
  echo "Compiling save_laz utility..."
  cmake -S "$PROJECT_ROOT/save_laz" -B "$PROJECT_ROOT/save_laz/build"
  cmake --build "$PROJECT_ROOT/save_laz/build" --config Release
  sudo install -m 0755 "$PROJECT_ROOT/save_laz/build/save_laz" /usr/local/bin/save_laz
else
  echo "save_laz utility already installed."
fi

if [ ! -d "$PROJECT_ROOT/.venv" ]; then
  echo "Creating Python virtual environment..."
  python3 -m venv "$PROJECT_ROOT/.venv"
fi
source "$PROJECT_ROOT/.venv/bin/activate"
pip install --upgrade pip
if ! pip show Flask >/dev/null 2>&1; then
  pip install Flask
fi

USB_PACKAGES=(exfatprogs ntfs-3g)
usb_missing=false
for pkg in "${USB_PACKAGES[@]}"; do
  if ! dpkg -s "$pkg" >/dev/null 2>&1; then
    usb_missing=true
    break
  fi
done

if [ "$usb_missing" = true ] || [ ! -f /usr/local/sbin/usb-automount.sh ] || [ ! -f /etc/udev/rules.d/99-usb-automount.rules ]; then
  echo "Installing USB auto-mount support..."
  "$PROJECT_ROOT/scripts/install_usbmount.sh"
else
  echo "USB auto-mount support already installed."
fi

CON_NAME="Wired connection 1"
if ! nmcli -g NAME con show | grep -Fx "$CON_NAME" >/dev/null 2>&1; then
  echo "Creating NetworkManager connection $CON_NAME..."
  sudo nmcli con add type ethernet ifname eth0 con-name "$CON_NAME"
fi
echo "Configuring static IP 192.168.6.1 on $CON_NAME..."
sudo nmcli con mod "$CON_NAME" ipv4.method manual ipv4.addresses 192.168.6.1/24
sudo nmcli con mod "$CON_NAME" ipv4.gateway ""
sudo nmcli con mod "$CON_NAME" ipv4.dns ""
sudo nmcli con mod "$CON_NAME" connection.autoconnect yes
sudo nmcli con mod "$CON_NAME" connection.interface-name eth0
sudo nmcli con up "$CON_NAME"

if [ ! -f /etc/systemd/system/tecscanner.service ]; then
  echo "Setting up Tecscanner systemd service..."
  "$PROJECT_ROOT/scripts/setup_service.sh"
else
  echo "Tecscanner systemd service already installed."
fi

echo "Installation complete."
