#!/usr/bin/env bash
set -euo pipefail

[[ $EUID -eq 0 ]] || { echo "Spusť skript pomocí sudo." >&2; exit 1; }
[[ "$(< /sys/class/dmi/id/product_name)" == MacBookPro14,2 ]] || {
  echo "Tento postup je určen pouze pro MacBookPro14,2." >&2
  exit 1
}
dir="$(cd -- "$(dirname -- "$0")" && pwd)"
script=/usr/local/libexec/omarchy-mbp-touchbar-rebind
service=/etc/systemd/system/omarchy-mbp-touchbar.service
for pair in "$dir/touchbar-rebind.sh:$script" "$dir/omarchy-mbp-touchbar.service:$service"; do
  source="${pair%%:*}"
  destination="${pair#*:}"
  if [[ -L "$destination" ]] || { [[ -e "$destination" ]] && ! cmp -s -- "$source" "$destination"; }; then
    echo "Soubor $destination již existuje a liší se; nebudu ho přepisovat." >&2
    exit 1
  fi
done
install -d -m 755 /usr/local/libexec
install -m 755 "$dir/touchbar-rebind.sh" "$script"
install -m 644 "$dir/omarchy-mbp-touchbar.service" "$service"
systemctl daemon-reload
systemctl enable --now omarchy-mbp-touchbar.service
