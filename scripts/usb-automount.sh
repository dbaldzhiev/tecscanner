#!/usr/bin/env bash
# Simple USB automount script triggered by udev.
# Mounts devices under /media/usbN on add and unmounts on remove.
set -e

MOUNT_ROOT=/media
DEVICE="/dev/$1"

if [ "$ACTION" = "add" ]; then
  mkdir -p "$MOUNT_ROOT"
  for N in $(seq 0 9); do
    MP="$MOUNT_ROOT/usb$N"
    if ! mount | grep -q " $MP "; then
      mkdir -p "$MP"
      mount "$DEVICE" "$MP"
      exit 0
    fi
  done
elif [ "$ACTION" = "remove" ]; then
  MP=$(mount | awk -v dev="$DEVICE" '$1==dev {print $3}')
  if [ -n "$MP" ]; then
    umount "$MP"
    rmdir "$MP"
  fi
fi
exit 0
