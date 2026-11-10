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
    for bad in [
        {"name": "x", "buttons": [{"label": "x", "command": ""}]},
        {"name": "x", "buttons": [{"label": "x", "kind": "widget", "command": "cpu", "width": 15.5}]},
        {"name": "x", "buttons": [{"label": "x", "kind": "process", "command": "firefox;id"}]},
        {"name": "x", "buttons": [], "typo": 1},
        {"name": "x", "buttons": [{"label": "x", "command": "true\0false"}]},
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
