#!/usr/bin/env bash
set -e

if [ ! -f /usr/local/sbin/usb-automount.sh ] || [ ! -f /etc/udev/rules.d/99-usb-automount.rules ]; then
  echo "Installing dependencies..."
  sudo apt update
  sudo apt install -y git debhelper

  echo "Cloning usbmount repository..."
  cd /tmp
  rm -rf usbmount
  git clone https://github.com/rbrito/usbmount.git
  cd usbmount

  echo "Building deb package..."
  dpkg-buildpackage -us -uc -b

  DEB=$(find .. -maxdepth 1 -name "usbmount_*.deb" | head -n1)
  echo "Installing package: $DEB"
  sudo dpkg -i "$DEB" || sudo apt --fix-broken install -y

  echo "Creating systemd override to fix PrivateMounts issue..."
  UDEV_DROP_DIR="/etc/systemd/system/systemd-udevd.service.d"
  sudo mkdir -p "$UDEV_DROP_DIR"
  cat <<'EOT' | sudo tee "$UDEV_DROP_DIR/00-usbmount-mountflags.conf"
[Service]
PrivateMounts=no
MountFlags=shared
EOT

  echo "Reloading systemd-udevd..."
  sudo systemctl daemon-reexec
  sudo systemctl restart systemd-udevd

  echo "Updating supported filesystems in usbmount config..."
  sudo sed -ri 's/^(FILESYSTEMS=).*/\1"vfat ext2 ext3 ext4 hfsplus ntfs exfat fuseblk"/' /etc/usbmount/usbmount.conf

  echo "Setting USB mount permissions for user access..."
  USER_UID=$(id -u)
  USER_GID=$(id -g)
  # shellcheck disable=SC2086
  sudo sed -ri "s|^(MOUNTOPTIONS=).*|\1"sync,noexec,nosuid,nodev,uid=${USER_UID},gid=${USER_GID},dmask=027,fmask=137"|" /etc/usbmount/usbmount.conf

  echo "Done. Please reboot for changes to fully apply."
else
  echo "USB auto-mount support already installed."
fi
