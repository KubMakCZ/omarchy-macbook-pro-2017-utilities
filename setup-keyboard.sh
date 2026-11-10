#!/usr/bin/env bash
set -euo pipefail

command -v hyprctl >/dev/null || { echo "Hyprland/hyprctl není dostupný." >&2; exit 1; }
hyprctl version >/dev/null || { echo "Spusť skript v běžící relaci Hyprlandu." >&2; exit 1; }

config_dir="${XDG_CONFIG_HOME:-$HOME/.config}/omarchy"
target="$config_dir/keyboard-layouts"
mkdir -p "$config_dir"
if [[ -L "$target" ]]; then
  echo "Soubor $target je symbolický odkaz; nebudu ho přepisovat." >&2
  exit 1
fi
if [[ -f "$target" ]] && [[ "$(<"$target")" == $'XKBLAYOUT=cz\nXKBVARIANT=' ]]; then
  echo "České rozložení je již nastavené."
  exit 0
fi
if [[ -e "$target" ]]; then
  backup="$(mktemp "$config_dir/keyboard-layouts.backup.XXXXXXXX")"
  cp -p -- "$target" "$backup"
  echo "Záloha: $backup"
fi

temp="$(mktemp "$config_dir/.keyboard-layouts.XXXXXXXX")"
trap 'rm -f -- "$temp"' EXIT
printf 'XKBLAYOUT=cz\nXKBVARIANT=\n' > "$temp"
mv -- "$temp" "$target"
hyprctl reload
errors="$(hyprctl configerrors)"
if [[ -n "$errors" ]]; then
  printf 'Hyprland hlásí chyby:\n%s\n' "$errors" >&2
  exit 1
fi
echo "Nastaveno české rozložení QWERTZ."
