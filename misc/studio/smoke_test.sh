#!/usr/bin/env bash
# Headless editor smoke test: opens a fresh project in the editor, lets it run
# for a number of frames, quits, and fails on crashes or Studio errors.
#
# Usage: misc/studio/smoke_test.sh <path/to/godot.editor.binary> [frames]
set -u

BIN="${1:?usage: smoke_test.sh <godot editor binary> [frames]}"
FRAMES="${2:-600}"

if [ ! -x "$BIN" ]; then
	echo "FAIL: '$BIN' is not executable"
	exit 1
fi

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
PROJECT="$WORK/project"
mkdir -p "$PROJECT"
cat > "$PROJECT/project.godot" <<'PROJ'
config_version=5

[application]

config/name="Studio Smoke Test"
PROJ

LOG="$WORK/editor.log"
# Keep editor settings/caches out of the user's home.
export XDG_CONFIG_HOME="$WORK/config" XDG_DATA_HOME="$WORK/data" XDG_CACHE_HOME="$WORK/cache"

# Two layout pages so the Studio pages bar starts with real data.
mkdir -p "$XDG_CONFIG_HOME/godot"
cat > "$XDG_CONFIG_HOME/godot/editor_layouts.cfg" <<'LAYOUTS'
[Scene Work]

dock_1="Scene,Import"
dock_5="Inspector"

[Code Work]

dock_3="FileSystem"
LAYOUTS

# First run imports the project (creates .godot/); second run is a normal editor session.
for pass in import editor; do
	if [ "$pass" = import ]; then
		ARGS=(--headless --editor --path "$PROJECT" --quit)
	else
		ARGS=(--headless --editor --path "$PROJECT" --quit-after "$FRAMES")
	fi
	timeout 600 "$BIN" "${ARGS[@]}" > "$LOG" 2>&1
	CODE=$?
	if [ $CODE -ne 0 ]; then
		echo "FAIL: editor ($pass pass) exited with code $CODE"
		tail -n 60 "$LOG"
		exit 1
	fi
	if grep -E -q "Segmentation fault|Program crashed|CrashHandler|ERROR: .*[Ss]tudio|SCRIPT ERROR" "$LOG"; then
		echo "FAIL: crash or Studio error in editor log ($pass pass)"
		grep -E -n "Segmentation fault|Program crashed|CrashHandler|ERROR|SCRIPT ERROR" "$LOG" | head -n 40
		exit 1
	fi
done

echo "OK: editor smoke test passed ($FRAMES frames)"
exit 0
