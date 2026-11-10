#!/usr/bin/env bash
set -euo pipefail

[[ $EUID -ne 0 ]] || { echo "Spusť aktivaci jako běžný uživatel." >&2; exit 1; }
config="${XDG_CONFIG_HOME:-$HOME/.config}"
[[ "$config" == /* ]] || { echo "XDG_CONFIG_HOME musí být absolutní cesta." >&2; exit 1; }
manager_env="$(systemctl --user show-environment)" || {
  echo "Uživatelský správce systemd není dostupný." >&2
  exit 1
}
manager_config="$(printf '%s\n' "$manager_env" | sed -n 's/^XDG_CONFIG_HOME=//p')"
manager_config="${manager_config:-$HOME/.config}"
[[ "$manager_config" == "$config" ]] || {
  echo "XDG_CONFIG_HOME se liší od nastavení uživatelské relace systemd; aktivaci nelze bezpečně provést." >&2
  exit 1
}
binary="$HOME/.local/bin/omarchy-glance"
[[ -f "$binary" && ! -L "$binary" && -x "$binary" ]] || {
  echo "Nejprve nainstaluj renderer pomocí bash setup.sh --renderer-build." >&2
  exit 1
}
binary="$(realpath -- "$binary")"
if [[ "$binary" == *"'"* || "$binary" == *'%'* || "$binary" == *'\'* || "$binary" == *$'\n'* ]]; then
  echo "Cesta rendereru obsahuje znaky, které nelze bezpečně použít ve službě." >&2
  exit 1
fi
if ! systemctl --user is-enabled --quiet t1-touchbar.service \
  || ! systemctl --user is-active --quiet t1-touchbar.service; then
  echo "Uživatelská služba t1-touchbar musí být povolená a aktivní; dokonči nejprve instalaci T1Bridge." >&2
  exit 1
fi

units="$config/systemd/user"
renderer_dir="$config/t1bridge"
mkdir -p -- "$units" "$renderer_dir"
service_tmp= socket_tmp= renderer_tmp=
trap 'rm -f -- "$service_tmp" "$socket_tmp" "$renderer_tmp"' EXIT
service_tmp="$(mktemp "$units/.omarchy-glance-service.XXXXXXXX")"
socket_tmp="$(mktemp "$units/.omarchy-glance-socket.XXXXXXXX")"
renderer_tmp="$(mktemp "$renderer_dir/.renderer.XXXXXXXX")"

cat > "$service_tmp" <<EOF
[Unit]
Description=omarchy-glance backend (widget data and actions for the Touch Bar and desktop row)
Requires=omarchy-glance.socket
After=omarchy-glance.socket

[Service]
ExecStart='$binary' backend
Restart=on-failure
EOF
cat > "$socket_tmp" <<'EOF'
[Unit]
Description=omarchy-glance backend socket

[Socket]
ListenStream=%t/omarchy-glance/backend.sock
SocketMode=0600
DirectoryMode=0700

[Install]
WantedBy=sockets.target
EOF
cat > "$renderer_tmp" <<EOF
#!/bin/sh
# Installed by \`omarchy-glance on\`.
exec '$binary' touchbar
EOF
chmod 644 "$service_tmp" "$socket_tmp"
chmod 755 "$renderer_tmp"
targets=("$units/omarchy-glance.service" "$units/omarchy-glance.socket" "$renderer_dir/renderer")
templates=("$service_tmp" "$socket_tmp" "$renderer_tmp")

for i in "${!targets[@]}"; do
  target="${targets[$i]}"
  if [[ -L "$target" ]] || { [[ -e "$target" ]] && { [[ ! -f "$target" ]] || ! cmp -s -- "${templates[$i]}" "$target"; }; }; then
    echo "Existující konfigurace se liší nebo je symbolický odkaz; nebudu ji přepisovat: $target" >&2
    exit 1
  fi
done
if [[ -f "${targets[2]}" && ! -x "${targets[2]}" ]]; then
  echo "Existující renderer není spustitelný; opravu proveď ručně." >&2
  exit 1
fi

changed=0
for i in "${!targets[@]}"; do
  if [[ ! -e "${targets[$i]}" ]]; then
    if ! ln -T -- "${templates[$i]}" "${targets[$i]}"; then
      echo "Konfigurace se mezitím změnila; aktivace nebyla dokončena." >&2
      exit 1
    fi
    changed=1
  fi
done
if (( ! changed )) && systemctl --user is-active --quiet omarchy-glance.socket; then
  echo "Renderer je již nastaven a socket je aktivní."
  exit 0
fi
systemctl --user daemon-reload
systemctl --user enable --now omarchy-glance.socket
systemctl --user try-restart omarchy-glance.service
systemctl --user restart t1-touchbar.service
echo "Renderer byl aktivován; ověř zobrazení a Esc na Touch Baru."
