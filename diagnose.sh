#!/usr/bin/env bash
set -u

echo "=== Model a jádro ==="
cat /sys/class/dmi/id/product_name 2>/dev/null || true
uname -r
echo
echo "=== Verze ==="
if command -v omarchy >/dev/null; then
  printf 'omarchy: %s\n' "$(omarchy version 2>/dev/null || echo neznámá)"
else
  echo "omarchy: nenalezeno"
fi
for package in linux-omarchy linux-omarchy-headers t1bridge t1bridge-dkms t1bridge-omarchy tbar; do
  if version="$(pacman -Q "$package" 2>/dev/null)"; then
    echo "$version"
  else
    echo "$package: nenainstalováno"
  fi
done
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
legacy=0 t1=0
for service in omarchy-mbp-touchbar.service tiny-dfr.service; do
  if systemctl is-active --quiet "$service" 2>/dev/null || systemctl is-enabled --quiet "$service" 2>/dev/null; then
    legacy=1
  fi
done
if [[ -d /sys/module/t1_cfgsel ]] || systemctl is-active --quiet t1-touchbar-hw.service 2>/dev/null; then
  t1=1
fi
if (( legacy && t1 )); then
  echo "POZOR: starý ovladač Touch Baru (omarchy-mbp-touchbar nebo tiny-dfr) je zapnutý současně s T1Bridge; viz docs/HARDWARE.md."
fi
echo
echo "=== Hyprland: klávesnice ==="
hyprctl getoption input:kb_layout 2>/dev/null || true
