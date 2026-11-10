#!/usr/bin/env bash
set -u

echo "=== Model a jádro ==="
cat /sys/class/dmi/id/product_name 2>/dev/null || true
uname -r
echo
echo "=== Wi-Fi hardware ==="
lspci -nnk 2>/dev/null | grep -i -A 4 'network controller' || true
echo
echo "=== Síťová rozhraní (bez názvů sítí) ==="
nmcli -t -f DEVICE,TYPE,STATE device 2>/dev/null || true
echo
echo "=== Apple USB zařízení ==="
for device in /sys/bus/usb/devices/*; do
  if [[ -f "$device/idVendor" ]] && [[ "$(<"$device/idVendor")" == 05ac ]]; then
    printf '05ac:%s\n' "$(<"$device/idProduct")"
  fi
done
echo
echo "=== Ovladače ==="
for module in applespi apple_ibridge apple_ib_tb t1_cfgsel appletbdrm apple_t1_ncm brcmfmac snd_hda_codec_cs8409; do
  if [[ -d "/sys/module/$module" ]]; then
    printf '%s: načten, ' "$module"
    modinfo -n "$module" 2>/dev/null || true
  else
    printf '%s: nenačten\n' "$module"
  fi
done
echo
echo "=== DKMS ==="
dkms status 2>/dev/null || true
echo
echo "=== Touch Bar služby ==="
for service in t1-touchbar-hw.service omarchy-mbp-touchbar.service tiny-dfr.service omarchy-mbp-usb-power.service; do
  state="$(systemctl is-active "$service" 2>/dev/null || true)"
  printf '%s: %s\n' "$service" "${state:-nedostupné}"
done
for service in t1-touchbar.service omarchy-glance.service; do
  state="$(systemctl --user is-active "$service" 2>/dev/null || true)"
  printf 'user/%s: %s\n' "$service" "${state:-nedostupné}"
done
echo
echo "=== Hyprland: klávesnice ==="
hyprctl getoption input:kb_layout 2>/dev/null || true
