#!/usr/bin/env bash
set -euo pipefail
dir="$(cd -- "$(dirname -- "$0")" && pwd)"

usage() {
  echo "Použití: bash setup.sh [--check|--keyboard|--usb-power|--build-app|--install-app|--renderer-build|--renderer-enable|--touchbar-service]"
}

if (( $# > 1 )); then usage >&2; exit 2; fi
case "${1:---check}" in
  -h|--help) usage ;;
  --check)
    bash "$dir/diagnose.sh"
    printf '\n=== Nástroje pro aplikaci ===\n'
    for tool in qmake6 make g++ git cargo rustc pkg-config; do
      if command -v "$tool" >/dev/null; then echo "$tool: dostupný"; else echo "$tool: chybí"; fi
    done
    if command -v pkg-config >/dev/null && pkg-config --exists cairo; then
      echo "cairo (vývojové soubory): dostupné"
    else
      echo "cairo (vývojové soubory): chybí"
    fi
    echo "Diagnostika nic neinstaluje. Úplný postup: docs/INSTALL.md."
    ;;
  --keyboard) bash "$dir/setup-keyboard.sh" ;;
  --build-app) bash "$dir/tbar/bin/build" ;;
  --install-app) bash "$dir/tbar/bin/install" ;;
  --renderer-build) bash "$dir/install-glance.sh" ;;
  --renderer-enable) bash "$dir/enable-glance.sh" ;;
  --usb-power)
    echo "Instalace opravy USB řadičů; vyžaduje oprávnění správce."
    exec sudo bash "$dir/install-usb-power-service.sh"
    ;;
  --touchbar-service)
    echo "Jen pro existující firmwareový ovladač; vyžaduje oprávnění správce."
    exec sudo bash "$dir/install-touchbar-service.sh"
    ;;
  *)
    usage >&2
    exit 2
    ;;
esac
