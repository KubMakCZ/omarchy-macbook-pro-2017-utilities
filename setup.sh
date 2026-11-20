#!/usr/bin/env bash
set -euo pipefail
dir="$(cd -- "$(dirname -- "$0")" && pwd)"

usage() {
  cat <<'EOF2'
Použití: bash setup.sh [VOLBA]

  --check             diagnostika a doporučený další krok, nic nemění (výchozí)
  --deps              nainstaluje balíčky pro sestavení rendereru a editoru; sudo
  --keyboard          české QWERTZ pro aktuálního uživatele, se zálohou
  --keyboard-check    změří zdvojování kláves, text neukládá; sudo
  --usb-power         volitelná oprava USB řadičů; sudo
  --no-suspend        vypne uspání do RAM, ze kterého se počítač neprobudí; sudo
  --suspend-restore   vrátí původní chování uspání; sudo
  --renderer-build    sestaví a nainstaluje renderer omarchy-glance, bez aktivace
  --renderer-enable   aktivuje renderer; vyžaduje funkční T1Bridge
  --build-app         sestaví editor tbar, bez instalace
  --install-app       sestaví a nainstaluje balíček tbar
  --touchbar-service  jen starý firmwareový ovladač; nikdy s T1Bridge; sudo
  -h, --help          tato nápověda

T1Bridge se instaluje ručně podle docs/INSTALL.md.
EOF2
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
    printf '\n=== Stav instalace ===\n'
    next=
    step() {
      if [[ "$1" == ok ]]; then echo "[hotovo] $2"; else echo "[chybí]  $2"; next="${next:-$3}"; fi
    }
    model="$(cat /sys/class/dmi/id/product_name 2>/dev/null || true)"
    if [[ "$model" != MacBookPro14,2 ]]; then
      echo "POZOR: model '${model:-neznámý}' není ověřený MacBookPro14,2; systémové kroky ho odmítnou."
    fi
    state=chybí
    if command -v pacman >/dev/null && pacman -Qq t1bridge t1bridge-dkms >/dev/null 2>&1; then state=ok; fi
    step "$state" "balíčky T1Bridge" "nainstaluj T1Bridge podle docs/INSTALL.md, část 3"
    state=chybí
    if [[ -d /sys/module/t1_cfgsel ]] && systemctl --user is-active --quiet t1-touchbar.service 2>/dev/null; then state=ok; fi
    step "$state" "T1Bridge běží (modul a uživatelská služba)" "restartuj a dokonči služby T1Bridge podle docs/INSTALL.md, část 3"
    state=ok
    for tool in qmake6 make g++ git cargo rustc pkg-config; do command -v "$tool" >/dev/null || state=chybí; done
    if ! command -v pkg-config >/dev/null || ! pkg-config --exists cairo; then state=chybí; fi
    step "$state" "nástroje pro sestavení" "bash setup.sh --deps"
    state=chybí
    if [[ -x "$HOME/.local/bin/omarchy-glance" ]]; then state=ok; fi
    step "$state" "renderer omarchy-glance sestaven" "bash setup.sh --renderer-build"
    state=chybí
    if [[ -e "${XDG_CONFIG_HOME:-$HOME/.config}/t1bridge/renderer" ]] \
      && systemctl --user is-active --quiet omarchy-glance.socket 2>/dev/null; then state=ok; fi
    step "$state" "renderer aktivní" "bash setup.sh --renderer-enable"
    state=chybí
    if command -v tbar >/dev/null; then state=ok; fi
    step "$state" "editor tbar nainstalován" "bash setup.sh --install-app"
    state=chybí
    if [[ "$(systemctl is-enabled suspend.target 2>/dev/null || true)" == masked ]]; then state=ok; fi
    step "$state" "ochrana před uspáním bez probuzení (volitelné)" "bash setup.sh --no-suspend"
    if [[ -n "$next" ]]; then
      echo "Další krok: $next"
    else
      echo "Vše je připraveno: spusť tbar a použij scénu."
    fi
    echo "Diagnostika nic neinstaluje. Úplný postup: docs/INSTALL.md."
    ;;
  --deps)
    packages=(base-devel git rust cairo pkgconf qt6-base qt6-declarative)
    echo "Nainstaluji chybějící balíčky: ${packages[*]}"
    echo "Databáze balíčků se neaktualizuje; při chybě nejdřív spusť omarchy update."
    exec sudo pacman -S --needed "${packages[@]}"
    ;;
  --keyboard-check)
    echo "Měření klávesnice vyžaduje oprávnění správce; napsaný text se neukládá."
    exec sudo python3 "$dir/key-chatter-check.py"
    ;;
  --no-suspend)
    echo "Vypnutí uspání do RAM; vyžaduje oprávnění správce."
    exec sudo bash "$dir/install-no-suspend.sh"
    ;;
  --suspend-restore)
    echo "Obnovení původního uspání; vyžaduje oprávnění správce."
    exec sudo bash "$dir/install-no-suspend.sh" --remove
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
