---
name: tbar
description: Safely inspect, validate, import and apply Touch Bar scenes with the tbar CLI.
---

# tbar

Use `tbar help` for commands and `tbar schema --json` for the single-profile
JSON schema. `tbar list --json` and `tbar show NAME --json` read saved scenes;
`tbar export NAME --json` previews renderer JSON without applying it.

Validate a proposed profile with `tbar validate FILE --json`, then use
`tbar import FILE --json` to add it. Import never applies it or changes the
selected scene. An existing name is rejected unless `--replace` is explicitly
supplied; replacement backs up the profiles file. Close or reload an open
editor before changing its profiles, and report a concurrency conflict rather
than overwriting edits.

Only run `tbar apply NAME --json` with the user's approval: it changes the
actual Touch Bar configuration and saves a backup. `tbar restore --json`
restores the original only if the applied layout has not changed externally.
Inspect all shell commands in profile JSON and do not execute untrusted ones.
Use temporary test files and never expose user data in examples or screenshots.
