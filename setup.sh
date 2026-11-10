#!/usr/bin/env bash
set -euo pipefail
dir="$(cd -- "$(dirname -- "$0")" && pwd)"

case "${1:---check}" in
  --check)
    bash "$dir/diagnose.sh"
    printf '\n=== Nástroje pro aplikaci ===\n'
    for tool in qmake6 make g++; do
      if command -v "$tool" >/dev/null; then echo "$tool: dostupný"; else echo "$tool: chybí"; fi
    done
    echo "Tento projekt zatím neinstaluje ani nemění ovladače."
    ;;
  --keyboard) bash "$dir/setup-keyboard.sh" ;;
  --build-app) bash "$dir/tbar/bin/build" ;;
  --touchbar-service)
    echo "Jen pro existující firmwareový ovladač; vyžaduje oprávnění správce."
    exec sudo bash "$dir/install-touchbar-service.sh"
    ;;
  *)
    echo "Použití: bash setup.sh [--check|--keyboard|--build-app|--touchbar-service]" >&2
    exit 2
    ;;
esac
