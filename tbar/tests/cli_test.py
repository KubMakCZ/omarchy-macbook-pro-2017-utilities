import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile


binary = str(Path(sys.argv[1]).resolve())

with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    env = os.environ.copy()
    env.update(HOME=str(root), XDG_CONFIG_HOME=str(root / "config"),
               XDG_STATE_HOME=str(root / "state"))
    env.pop("DISPLAY", None)
    env.pop("WAYLAND_DISPLAY", None)

    def run(*args, input=None, success=True):
        result = subprocess.run([binary, *args], input=input, text=True, env=env,
                                capture_output=True, timeout=10)
        assert (result.returncode == 0) == success, (args, result.stderr, result.stdout)
        return json.loads(result.stdout if success else result.stderr)

    profile = {"name": "Demo", "theme": "auto", "buttons": [
        {"label": "Keys", "command": "omarchy menu keybindings", "section": "center"},
        {"kind": "widget", "label": "CPU", "command": "cpu", "style": "bars",
         "interval": 1, "temperature": True, "width": 100}]}
    profile_file = root / "profile.json"
    profile_file.write_text(json.dumps(profile))
    config = root / "config"
    assert run("schema", "--json")["title"] == "tbar profile"
    assert run("list", "--json")["selected"] == "Omarchy"
    assert run("validate", str(profile_file), "--json")["ok"]
    assert not config.exists()
    assert run("import", "-", "--json", input=json.dumps(profile))["ok"]
    assert run("list")["selected"] == "Omarchy"
    assert run("show", "Demo") == profile
    assert "layers" in run("export", "Demo", "--json")
    assert not (config / "omarchy-glance" / "touchbar.json").exists()
    before = (config / "tbar" / "profiles.json").read_bytes()
    run("import", str(profile_file), "--json", success=False)
    assert (config / "tbar" / "profiles.json").read_bytes() == before
    profile["buttons"][0]["command"] = "true"
    profile_file.write_text(json.dumps(profile))
    run("import", str(profile_file), "--replace", "--json")
    backups = list((config / "tbar").glob("profiles.json.backup-*"))
    assert len(backups) == 1 and backups[0].read_bytes() == before
    run("apply", "Demo", "--json")
    assert run("list")["selected"] == "Omarchy"
    renderer = config / "omarchy-glance" / "touchbar.json"
    assert renderer.exists()
    run("restore", "--json")
    assert not renderer.exists()
    run("select", "Demo", "--json")
    assert run("list")["selected"] == "Demo"
    # Scene switching: refused until a layout was applied, then cycles without new backups.
    assert run("list")["applied"] == ""
    run("next", "--json", success=False)
    assert not renderer.exists()
    run("apply", "Omarchy", "--json")
    assert run("list")["applied"] == "Omarchy"
    assert run("next", "--json")["name"] == "Demo"
    assert run("next", "--json")["name"] == "Omarchy"
    assert not list(renderer.parent.glob("touchbar.json.backup-*"))
    web = {"name": "Web", "apps": ["firefox"], "buttons": [
        {"kind": "control", "label": "Volume", "command": "volume"},
        {"kind": "scene-next", "label": "Next"}]}
    run("import", "-", "--json", input=json.dumps(web))
    # tbar watch follows Hyprland's focus events from its event socket.
    import socket
    import time
    runtime = root / "run"
    (runtime / "hypr" / "test").mkdir(parents=True)
    server = socket.socket(socket.AF_UNIX)
    server.bind(str(runtime / "hypr" / "test" / ".socket2.sock"))
    server.listen(1)
    watch_env = dict(env, XDG_RUNTIME_DIR=str(runtime), HYPRLAND_INSTANCE_SIGNATURE="test")
    watcher = subprocess.Popen([binary, "watch"], env=watch_env, stderr=subprocess.PIPE, text=True)
    try:
        server.settimeout(10)
        connection, _ = server.accept()

        def focus(window_class, expected):
            connection.sendall(f"workspace>>1\nactivewindow>>{window_class},Some, title\n".encode())
            for _ in range(100):
                if run("list")["applied"] == expected:
                    return
                time.sleep(0.05)
            raise AssertionError((window_class, expected, run("list")))

        focus("Firefox", "Web")
        focus("kitty", "Omarchy")
        focus("firefox", "Web")
        # A layout picked by hand stays while the same application keeps focus.
        assert run("next", "--json")["name"] == "Omarchy"
        focus("firefox", "Omarchy")
        focus("", "Omarchy")
        connection.close()
        assert watcher.wait(timeout=10) == 1
    finally:
        watcher.kill()
        server.close()
    assert not list(renderer.parent.glob("touchbar.json.backup-*"))
    run("restore", "--json")
    assert run("list")["applied"] == ""
    for bad in [
        {"name": "x", "buttons": [{"label": "x", "command": ""}]},
        {"name": "x", "buttons": [{"label": "x", "kind": "widget", "command": "cpu", "width": 15.5}]},
        {"name": "x", "buttons": [{"label": "x", "kind": "process", "command": "firefox;id"}]},
        {"name": "x", "buttons": [], "typo": 1},
        {"name": "x", "buttons": [{"label": "x", "command": "true\0false"}]},
        {"name": "x", "buttons": [{"label": "x", "kind": "control", "command": "slider"}]},
        {"name": "x", "apps": ["fire fox"], "buttons": []},
    ]:
        run("validate", "-", "--json", input=json.dumps(bad), success=False)
    run("validate", "-", "--json", input="[]", success=False)
    run("import", "-", "--json", input="x" * (1024 * 1024 + 1), success=False)
    run("unknown", "--json", success=False)
    run("export", "Demo", "extra", "--json", success=False)
    run("skill", "install", "--json")
    skill = root / ".agents" / "skills" / "tbar" / "SKILL.md"
    original = skill.read_bytes()
    run("skill", "install", "--json", success=False)
    assert skill.read_bytes() == original
print("CLI integration tests passed")
