#!/bin/sh
# Runs the in-app UI click test with the window forced to the foreground (macOS:
# keyboard shortcut checks fail if another app, e.g. an IDE, holds focus).
# Usage: tools/run-ui-test.sh [media-file>=40s]   (default /tmp/long.mp4)
BIN="${LUMINA_BIN:-build/mac/bin/lumina}"
MEDIA="${1:-/tmp/long.mp4}"
LOG="$(mktemp)"
LUMINA_UI_TEST=1 "$BIN" "$MEDIA" >"$LOG" 2>&1 &
PID=$!
while kill -0 "$PID" 2>/dev/null; do
  [ "$(uname)" = Darwin ] && osascript -e "tell application \"System Events\" to set frontmost of first process whose unix id is $PID to true" 2>/dev/null
  sleep 2
done
grep -E "UITEST (FAIL|SKIP|SUMMARY)" "$LOG"
grep -q "fails= 0" "$LOG"
