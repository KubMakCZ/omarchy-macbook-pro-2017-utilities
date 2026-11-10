#!/usr/bin/env bash
set -euo pipefail

revision=9c75e5e1d71cf583fe9b02c124cffa272fd7fcb1
url=https://github.com/andersryanc/omarchy-macbook-touchbar-custom.git
source_dir="${XDG_CACHE_HOME:-$HOME/.cache}/omarchy-mbp/omarchy-glance"
bin_dir="$HOME/.local/bin"
target="$bin_dir/omarchy-glance"

[[ $EUID -ne 0 ]] || { echo "Spusť sestavení jako běžný uživatel, ne jako root." >&2; exit 1; }
for tool in git cargo rustc pkg-config; do
  command -v "$tool" >/dev/null || { echo "Chybí nástroj: $tool" >&2; exit 1; }
done
pkg-config --exists cairo || { echo "Chybí vývojové soubory cairo." >&2; exit 1; }
if [[ -L "$source_dir" ]]; then
  echo "Zdrojový adresář je symbolický odkaz; nebudu ho měnit." >&2
  exit 1
fi
if [[ ! -e "$source_dir" ]]; then
  mkdir -p -- "$(dirname -- "$source_dir")"
  git clone --no-checkout "$url" "$source_dir"
  git -C "$source_dir" checkout --detach "$revision"
fi
if [[ ! -d "$source_dir/.git" ]] \
  || [[ "$(git -C "$source_dir" rev-parse --show-toplevel)" != "$source_dir" ]] \
  || [[ "$(git -C "$source_dir" rev-parse HEAD)" != "$revision" ]] \
  || [[ -n "$(git -C "$source_dir" status --porcelain --untracked-files=normal)" ]]; then
  echo "Zdrojový adresář není čistý checkout požadované revize; nic v něm neměním: $source_dir" >&2
  exit 1
fi
cargo build --locked --release --manifest-path "$source_dir/Cargo.toml" --target-dir "$source_dir/target"
binary="$source_dir/target/release/omarchy-glance"
[[ -f "$binary" && ! -L "$binary" ]] || { echo "Sestavený renderer chybí." >&2; exit 1; }
mkdir -p -- "$bin_dir"
if [[ -e "$target" || -L "$target" ]]; then
  if [[ -f "$target" && ! -L "$target" ]] && cmp -s -- "$binary" "$target"; then
    echo "Stejný renderer je již nainstalovaný: $target"
    exit 0
  fi
  echo "Cíl $target již existuje a liší se; nebudu ho přepisovat." >&2
  exit 1
fi
temp="$(mktemp "$bin_dir/.omarchy-glance.XXXXXXXX")"
trap 'rm -f -- "$temp"' EXIT
install -m 755 -- "$binary" "$temp"
if ! ln -T -- "$temp" "$target"; then
  echo "Renderer nebyl nainstalován; cílový soubor se mohl mezitím objevit." >&2
  exit 1
fi
echo "Renderer nainstalován: $target"
echo "Nebyl aktivován. Před aktivací si přečti docs/INSTALL.md a zazálohuj existující konfiguraci."
