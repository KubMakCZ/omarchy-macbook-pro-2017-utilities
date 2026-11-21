#!/usr/bin/env bash
# Vypne uspání do RAM (suspend), ze kterého se MacBookPro14,2 s T1Bridge
# neprobudí. Hibernace a vypnutí zůstávají dostupné. Návrat: --remove.
set -euo pipefail

# OMARCHY_MBP_TEST_ROOT slouží jen testům: zapisuje se pod tento adresář
# a kontrola oprávnění a modelu se přeskočí. sudo tuto proměnnou nepředává.
root="${OMARCHY_MBP_TEST_ROOT:-}"
if [[ -z "$root" ]]; then
  [[ $EUID -eq 0 ]] || { echo "Spusť skript pomocí sudo." >&2; exit 1; }
  [[ "$(< /sys/class/dmi/id/product_name)" == MacBookPro14,2 ]] || {
    echo "Tento postup je určen pouze pro MacBookPro14,2." >&2
    exit 1
  }
fi
(( $# <= 1 )) || { echo "Použití: install-no-suspend.sh [--remove]" >&2; exit 2; }

logind_config="$root/etc/systemd/logind.conf.d/60-omarchy-mbp-no-suspend.conf"
sleep_config="$root/etc/systemd/sleep.conf.d/60-omarchy-mbp-no-suspend.conf"
targets=(suspend.target suspend-then-hibernate.target hybrid-sleep.target)
temp="$(mktemp -d)"
trap 'rm -rf -- "$temp"' EXIT
cat > "$temp/logind" <<'CONFIG'
# Nainstaloval omarchy-macbook-pro-2017-utilities (install-no-suspend.sh).
# Zavření víka ani klávesa uspání počítač neuspí: probuzení na tomto modelu
# s T1Bridge nefunguje a končí vynuceným vypnutím.
[Login]
HandleLidSwitch=ignore
HandleLidSwitchExternalPower=ignore
HandleLidSwitchDocked=ignore
HandleSuspendKey=ignore
CONFIG
cat > "$temp/sleep" <<'CONFIG'
# Nainstaloval omarchy-macbook-pro-2017-utilities (install-no-suspend.sh).
# Systém uspání do RAM vůbec nenabízí, takže ho nezvolí ani UPower při téměř
# vybité baterii (použije hibernaci nebo vypnutí) ani nabídka či nečinnost.
[Sleep]
AllowSuspend=no
AllowSuspendThenHibernate=no
AllowHybridSleep=no
CONFIG

for name in logind sleep; do
  config="${name}_config"
  if [[ -L "${!config}" ]] || { [[ -e "${!config}" ]] && ! cmp -s -- "$temp/$name" "${!config}"; }; then
    echo "Soubor ${!config} již existuje a liší se; nic nebylo změněno." >&2
    exit 1
  fi
done
for target in "${targets[@]}"; do
  link="$root/etc/systemd/system/$target"
  if [[ -e "$link" || -L "$link" ]] && [[ "$(readlink -- "$link" 2>/dev/null)" != /dev/null ]]; then
    echo "Jednotka $link již existuje a není to maska; nic nebylo změněno." >&2
    exit 1
  fi
done

case "${1:-}" in
  "")
    install -D -m 644 -- "$temp/logind" "$logind_config"
    install -D -m 644 -- "$temp/sleep" "$sleep_config"
    systemctl mask "${targets[@]}"
    systemctl reload systemd-logind.service
    echo "Uspání do RAM je vypnuté; zavření víka počítač neuspí."
    echo "Při téměř vybité baterii systém místo uspání hibernuje nebo se vypne."
    echo "Displej může pod zavřeným víkem zůstat zapnutý: před zavřením zamkni nebo vypni počítač."
    ;;
  --remove)
    if [[ ! -e "$logind_config" && ! -e "$sleep_config" ]]; then
      echo "Konfigurace není nainstalovaná; nic nebylo změněno." >&2
      exit 1
    fi
    rm -f -- "$logind_config" "$sleep_config"
    systemctl unmask "${targets[@]}"
    systemctl reload systemd-logind.service
    echo "Původní chování uspání je obnoveno."
    ;;
  *)
    echo "Použití: install-no-suspend.sh [--remove]" >&2
    exit 2
    ;;
esac
