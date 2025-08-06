#!/usr/bin/env bash
# Simple USB automount script triggered by udev.
# Mounts devices under /media/usbN on add and unmounts on remove.
# The mount root can be overridden with the MOUNT_ROOT env var or a second arg.

set -u -o pipefail

# Allow overriding the mount root via the second argument or an env var
MOUNT_ROOT=${2:-${MOUNT_ROOT:-/media}}
DEVICE="/dev/$1"

log() {
  logger -t usb-automount "$*"
}

if [ "$ACTION" = "add" ]; then
  mkdir -p "$MOUNT_ROOT"
  for N in $(seq 0 9); do
    MP="$MOUNT_ROOT/usb$N"
    if ! mount | grep -q " $MP "; then
      mkdir -p "$MP"
      if mount "$DEVICE" "$MP"; then
        log "mounted $DEVICE at $MP"
        exit 0
      else
        log "failed to mount $DEVICE at $MP"
        rmdir "$MP"
      fi
    fi
  done
  log "no mount point available for $DEVICE"
  exit 1
elif [ "$ACTION" = "remove" ]; then
  MP=$(mount | awk -v dev="$DEVICE" '$1==dev {print $3}')
  if [ -n "$MP" ]; then
    if umount "$MP"; then
      log "unmounted $DEVICE from $MP"
      rmdir "$MP" || log "failed to remove $MP"
      exit 0
    else
      log "failed to unmount $DEVICE from $MP"
      exit 1
    fi
  else
    log "device $DEVICE not mounted"
    exit 1
  fi
else
  log "unsupported action: $ACTION for $DEVICE"
  exit 1
fi
