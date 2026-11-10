#!/usr/bin/env python3
import os
from pathlib import Path
import shutil
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
    systemctl = tools / "systemctl"
    systemctl.write_text("""#!/bin/sh
printf '%s\\n' "$*" >> "$HOME/systemctl.log"
case "$*" in
    *show-environment*) printf 'XDG_CONFIG_HOME=%s\\n' "${TEST_MANAGER_CONFIG:-$XDG_CONFIG_HOME}" ;;
    *t1-touchbar.service*) [ "${TEST_T1_DOWN:-0}" = 0 ] || exit 1 ;;
esac
exit 0
""")
    systemctl.chmod(0o755)
    env = os.environ.copy()
    env.update(HOME=str(home), XDG_CACHE_HOME=str(cache),
               XDG_CONFIG_HOME=str(home / ".config"),
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
    run("--renderer-enable", expected=1, extra_env={"TEST_MANAGER_CONFIG": str(root / "other")})
    assert not (home / ".config/systemd/user/omarchy-glance.service").exists()
    inactive_config = root / "inactive-config"
    run("--renderer-enable", expected=1,
        extra_env={"TEST_T1_DOWN": "1", "XDG_CONFIG_HOME": str(inactive_config)})
    assert not inactive_config.exists()
    conflict_config = root / "conflict-config"
    conflict_file = conflict_config / "t1bridge/renderer"
    conflict_file.parent.mkdir(parents=True)
    conflict_file.write_text("custom renderer")
    run("--renderer-enable", expected=1, extra_env={"XDG_CONFIG_HOME": str(conflict_config)})
    assert conflict_file.read_text() == "custom renderer"
    assert not (conflict_config / "systemd/user/omarchy-glance.service").exists()
    conflict_file.unlink()
    conflict_file.symlink_to(home / "original")
    run("--renderer-enable", expected=1, extra_env={"XDG_CONFIG_HOME": str(conflict_config)})
    assert conflict_file.is_symlink()
    run("--renderer-enable")
    config = home / ".config"
    assert (config / "t1bridge/renderer").read_text().endswith(
        f"exec '{target}' touchbar\n")
    assert (config / "systemd/user/omarchy-glance.service").is_file()
    assert (config / "systemd/user/omarchy-glance.socket").is_file()
    log = (home / "systemctl.log").read_text()
    assert log.count("restart t1-touchbar.service") == 1
    run("--renderer-enable")
    assert (home / "systemctl.log").read_text().count("restart t1-touchbar.service") == 1
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

real_binary = os.environ.get("GLANCE_BINARY")
if real_binary:
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        home, tools = root / "home", root / "tools"
        binary = home / ".local/bin/omarchy-glance"
        binary.parent.mkdir(parents=True)
        tools.mkdir()
        shutil.copy2(real_binary, binary)
        systemctl = tools / "systemctl"
        systemctl.write_text("""#!/bin/sh
case "$*" in
    *show-environment*) printf 'XDG_CONFIG_HOME=%s\\n' "$XDG_CONFIG_HOME" ;;
esac
exit 0
""")
        systemctl.chmod(0o755)
        env = os.environ.copy()
        env.update(HOME=str(home), PATH=str(tools) + os.pathsep + env["PATH"])
        actual, expected = root / "actual", root / "expected"
        env["XDG_CONFIG_HOME"] = str(actual)
        subprocess.run(["bash", str(project / "setup.sh"), "--renderer-enable"],
                       env=env, check=True, capture_output=True, timeout=30)
        env["XDG_CONFIG_HOME"] = str(expected)
        subprocess.run([str(binary), "on"], env=env, check=True, capture_output=True, timeout=30)
        for path in ("t1bridge/renderer", "systemd/user/omarchy-glance.service",
                     "systemd/user/omarchy-glance.socket"):
            assert (actual / path).read_bytes() == (expected / path).read_bytes(), path
    print("Renderer activation matches pinned upstream")
print("Installer safety tests passed")
