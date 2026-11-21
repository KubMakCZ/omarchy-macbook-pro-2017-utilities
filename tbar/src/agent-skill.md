---
name: tbar
description: Create, inspect, validate, import and apply MacBook Touch Bar scenes (buttons, status items, live graphs) with the tbar CLI on Omarchy. Use when the user asks to add or change Touch Bar buttons, scenes or layouts.
---

# tbar

`tbar` stores Touch Bar scenes ("profiles") and exports them to the
omarchy-glance renderer. Every command below works without a display and
prints JSON; with `--json`, errors are JSON on stderr and the exit code is
non-zero. Run `tbar help` for the command list and `tbar schema --json` for
the authoritative single-profile JSON Schema.

## Workflow

1. Read: `tbar list --json` (names and the selected scene), then
   `tbar show NAME --json` for the scene you will change.
2. Write one profile object to a temporary file, not to `profiles.json`.
3. Check: `tbar validate FILE --json`. Fix every reported error.
4. Save: `tbar import FILE --json`. An existing name is rejected unless you
   pass `--replace`, which backs up the profiles file first. Import never
   applies the scene or changes the selected one.
5. Optionally preview the renderer output with `tbar export NAME --json`.
6. Apply only with the user's approval: `tbar apply NAME --json` changes the
   real Touch Bar and saves a backup of the renderer configuration.

`tbar next --json` switches the Touch Bar to the following scene. It and the
`scene` buttons work only after one `tbar apply`, and refuse when the renderer
configuration was changed outside tbar.

`tbar select NAME --json` changes the scene selected in the editor, not the
Touch Bar. `tbar restore --json` returns to the layout that existed before
tbar was first used, and refuses if that layout was changed outside tbar.

## Profile format

```json
{
  "name": "Demo",
  "theme": "auto",
  "buttons": [
    { "label": "Keys", "command": "omarchy menu keybindings", "width": 150 },
    { "kind": "process", "label": "Firefox", "command": "firefox", "section": "center" },
    { "kind": "spacer", "label": "Spacer", "command": "", "width": 40 },
    { "kind": "widget", "label": "CPU", "command": "cpu", "section": "right",
      "width": 100, "style": "bars", "interval": 1, "temperature": true }
  ]
}
```

- `name`: 1-40 characters, no leading or trailing whitespace.
- `theme`: omit it, or `"auto"` to colour buttons from the Omarchy theme.
- `apps`: optional, at most 8 Hyprland window classes (`A-Za-z0-9._+-`,
  case-insensitive, see `hyprctl activewindow`). While the user's
  `tbar-watch.service` runs, the scene is shown when such a window is focused
  and the last manually chosen scene returns afterwards. Do not enable the
  service yourself without approval.
- `buttons`: at most 12 items. Escape and the Fn layer (F1-F12) are added
  automatically; do not add them yourself.
- `label`: required, 1-16 characters, single line.
- `kind`: omitted or `button` (`command` is a shell command, required),
  `widget` (`command` is `cpu`, `memory`, `network`, `disk` or `battery`),
  `process` (`command` is an exact process name, 1-15 characters of
  `A-Za-z0-9._+-`), `internet`, `temperature`, `spacer`, `control`
  (`command` is `volume`, `brightness`, `keyboard`, `media` or `mic`; one item
  becomes a group of 1-4 keys, there are no sliders), `scene` (`command` is
  the name of the scene to switch to), `scene-next` (no `command`). Give every
  scene reachable by a `scene` button its own button back.
- `section`: `left` (default), `center` or `right`. Sections are positioned
  independently and overlap when too wide; keep the total width modest.
- `width`: integer 10-500. `style` (`dots` or `bars`) and `interval`
  (1-60 seconds) are valid only on widgets; `temperature` (boolean) only on
  the `cpu` widget. Unknown fields are rejected.

## Safety

- Button commands run as the user in `/bin/sh`. Read every command in a
  profile before importing or applying it, and never add commands from an
  untrusted source or commands the user did not ask for.
- Status text can stay visible on a locked screen; do not put secrets or
  personal data in labels or process names.
- If a command reports a concurrency conflict, stop and tell the user to close
  or reload the editor. Do not edit `~/.config/tbar/profiles.json` or the
  renderer configuration by hand to work around it.
- When testing, use an isolated `HOME` and `XDG_CONFIG_HOME` so the user's
  scenes and active Touch Bar layout are untouched.
