#!/bin/bash
set -euo pipefail
binary="$1"
tmp="$(mktemp -d)"
trap 'rm -f -- "$tmp/omarchy-shell" "$tmp/omarchy-hyprland-session-locked" "$tmp/hit"; rmdir -- "$tmp"' EXIT
cat > "$tmp/omarchy-shell" <<'EOF'
#!/bin/sh
printf '%s\n' "${TBAR_TEST_LOCKED:-false}"
EOF
cat > "$tmp/omarchy-hyprland-session-locked" <<'EOF'
#!/bin/sh
exit "${TBAR_TEST_LOCK_EXIT:-1}"
EOF
chmod 700 "$tmp/omarchy-shell" "$tmp/omarchy-hyprland-session-locked"
export PATH="$tmp:$PATH"
payload="$(printf 'touch %s/hit' "$tmp" | base64 -w0)"
TBAR_TEST_LOCKED=true "$binary" action "$payload" && exit 1
TBAR_TEST_LOCK_EXIT=2 "$binary" action "$payload" && exit 1
test ! -e "$tmp/hit"
"$binary" action "$payload"
for _ in {1..50}; do
  test -e "$tmp/hit" && exit 0
  sleep 0.02
done
exit 1
