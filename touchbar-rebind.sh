#!/usr/bin/env bash
set -euo pipefail

[[ "$(< /sys/class/dmi/id/product_name)" == MacBookPro14,2 ]] || {
  echo "Tento postup je určen pouze pro MacBookPro14,2." >&2
  exit 1
}

target=/sys/bus/hid/drivers/apple-ibridge-hid
shopt -s nullglob
for ((attempt=0; attempt<30; attempt++)); do
  devices=(/sys/bus/hid/devices/0003:05AC:8600.*)
  if [[ -d "$target" ]] && ((${#devices[@]} >= 2)); then
    break
  fi
  sleep 1
done
[[ -d "$target" ]] || { echo "Ovladač apple-ibridge-hid není načten." >&2; exit 1; }
((${#devices[@]} >= 2)) || { echo "Nenalezena obě rozhraní iBridge." >&2; exit 1; }

count=0
for device in "${devices[@]}"; do
  id="${device##*/}"
  current="$(basename "$(readlink -f "$device/driver" 2>/dev/null || echo unbound)")"
  case "$current" in
    apple-ibridge-hid) ;;
    hid-generic|hid-sensor-hub)
      printf '%s' "$id" > "/sys/bus/hid/drivers/$current/unbind"
      if ! printf '%s' "$id" > "$target/bind"; then
        printf '%s' "$id" > "/sys/bus/hid/drivers/$current/bind" || true
        echo "Připojení $id selhalo." >&2
        exit 1
      fi
      ;;
    *) echo "Neznámý ovladač $current u $id; neprovádím změnu." >&2; exit 1 ;;
  esac
  ((count+=1))
done
((count >= 2)) || { echo "Nenalezena obě rozhraní iBridge." >&2; exit 1; }
echo "Rozhraní iBridge jsou připojena k apple-ibridge-hid."
