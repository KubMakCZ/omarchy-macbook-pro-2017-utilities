#!/usr/bin/env bash
set -euo pipefail

[[ "$(< /sys/class/dmi/id/product_name)" == MacBookPro14,2 ]] || {
  echo "Tento postup je určen pouze pro MacBookPro14,2." >&2
  exit 1
}

for address in 0000:06:00.0 0000:7c:00.0; do
  device="/sys/bus/pci/devices/$address"
  if [[ ! -r "$device/vendor" || ! -r "$device/device" ||
        "$(<"$device/vendor")" != 0x8086 || "$(<"$device/device")" != 0x15d4 ]]; then
    echo "Očekávaný USB řadič $address nebyl nalezen; neprovádím změny." >&2
    exit 1
  fi
done

for address in 0000:06:00.0 0000:7c:00.0; do
  printf 'on\n' > "/sys/bus/pci/devices/$address/power/control"
done
echo "Runtime uspávání obou USB řadičů je vypnuté."
