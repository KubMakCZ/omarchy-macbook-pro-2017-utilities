#!/usr/bin/env python3
import os
from pathlib import Path
import subprocess
import tempfile


project = Path(__file__).resolve().parent.parent
revision = "9c75e5e1d71cf583fe9b02c124cffa272fd7fcb1"
with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    home, cache, tools = root / "home", root / "cache", root / "bin"
    home.mkdir()
    cache.mkdir()
    tools.mkdir()
    git = tools / "git"
    git.write_text("""#!/bin/sh
set -eu
if [ "$1" = clone ]; then
    for arg do destination="$arg"; done
    mkdir -p "$destination/.git"
    touch "$destination/Cargo.toml"
    exit 0
fi
[ "$1" = -C ]
directory=$2
shift 2
case "$1 $2" in
    "checkout --detach") exit 0 ;;
    "rev-parse --show-toplevel") printf '%s\\n' "$directory" ;;
    "rev-parse HEAD") printf '%s\\n' "$TEST_REVISION" ;;
    "status --porcelain") [ "${TEST_DIRTY:-0}" = 0 ] || echo ' M changed' ;;
    *) exit 1 ;;
esac
""")
    cargo = tools / "cargo"
    cargo.write_text("""#!/bin/sh
set -eu
while [ "$#" -gt 0 ]; do
    if [ "$1" = --target-dir ]; then
        mkdir -p "$2/release"
        printf 'renderer test binary\\n' > "$2/release/omarchy-glance"
        exit 0
    fi
    shift
done
exit 1
""")
    git.chmod(0o755)
    cargo.chmod(0o755)
    makepkg = tools / "makepkg"
    makepkg.write_text('#!/bin/sh\nprintf "%s\\n" "$*" >> "$HOME/makepkg.log"\n')
    makepkg.chmod(0o755)
    omarchy = tools / "omarchy"
    omarchy.write_text('#!/bin/sh\nprintf "%s\\n" "$*" >> "$HOME/omarchy.log"\n')
    omarchy.chmod(0o755)
    env = os.environ.copy()
    env.update(HOME=str(home), XDG_CACHE_HOME=str(cache),
               PATH=str(tools) + os.pathsep + env["PATH"], TEST_REVISION=revision)

    def run(*args, expected=0, extra_env=None):
        test_env = env.copy()
        test_env.update(extra_env or {})
        result = subprocess.run(["bash", str(project / "setup.sh"), *args], env=test_env,
                                text=True, capture_output=True, timeout=30)
        assert result.returncode == expected, (args, result.stdout, result.stderr)
        return result

    run("--help")
    run("--check", "extra", expected=2)
    run("--install-app")
    assert (home / "makepkg.log").read_text().strip() == "-fsi"
    assert "hook install theme-set" in (home / "omarchy.log").read_text()
    hook = home / ".config/omarchy/hooks/theme-set.d/tbar-theme-sync"
    hook.parent.mkdir(parents=True)
    hook.write_text("custom hook")
    run("--install-app")
    assert hook.read_text() == "custom hook"
    assert len((home / "omarchy.log").read_text().splitlines()) == 1
    run("--renderer-build")
    target = home / ".local/bin/omarchy-glance"
    assert target.read_text() == "renderer test binary\n"
    inode = target.stat().st_ino
    run("--renderer-build")
    assert target.stat().st_ino == inode
    target.write_text("existing custom renderer")
    run("--renderer-build", expected=1)
    assert target.read_text() == "existing custom renderer"
    run("--renderer-build", expected=1, extra_env={"TEST_DIRTY": "1"})
    assert target.read_text() == "existing custom renderer"
    target.unlink()
    original = home / "original"
    original.write_text("unchanged")
    target.symlink_to(original)
    run("--renderer-build", expected=1)
    assert target.is_symlink() and original.read_text() == "unchanged"
print("Installer safety tests passed")
