#!/bin/sh
# Collects the information needed to debug audio/video problems on Linux.
# Usage: tools/diagnose-linux.sh /path/to/movie.mkv [path/to/lumina]
# Paste the whole output back (it contains no personal data besides file names).
MOVIE="$1"
BIN="${2:-build/linux-debug/lumina}"
[ -x "$BIN" ] || BIN="$(dirname "$BIN")/bin/lumina"   # older builds put it in bin/
[ -z "$MOVIE" ] || [ ! -f "$MOVIE" ] && { echo "usage: $0 /real/path/to/movie.mkv [lumina-binary]  (file not found: $MOVIE)"; exit 1; }
sec() { printf '\n===== %s =====\n' "$1"; }

sec "system"
cat /etc/os-release 2>/dev/null | grep -E '^(PRETTY_NAME|VERSION_ID)='
uname -r
echo "session: ${XDG_SESSION_TYPE:-?}  desktop: ${XDG_CURRENT_DESKTOP:-?}"

sec "packages (ffmpeg / mpv / qt / audio)"
rpm -qa 2>/dev/null | grep -iE '^(ffmpeg|libavcodec|libavformat|mpv|libmpv|qt6-qtbase-[0-9]|qt6-qtdeclarative-[0-9]|pipewire|wireplumber|pulseaudio|mesa-va|mesa-vdpau|libva)' | sort
dnf repolist 2>/dev/null | grep -iE 'rpmfusion' || echo "rpmfusion: NOT enabled"

sec "audio server"
(pactl info 2>&1 | grep -E 'Server (Name|Version)|Default Sink') || echo "pactl not available"
(wpctl status 2>&1 | sed -n 1,25p) || true

sec "movie streams"
ffprobe -hide_banner "$MOVIE" 2>&1 | grep -E 'Stream|Duration'

sec "can this ffmpeg decode the audio codecs? (needs to list the decoder)"
for c in aac ac3 eac3 dca truehd flac opus vorbis mp3; do   # dca = DTS
  printf '%-8s ' "$c"; ffmpeg -hide_banner -decoders 2>/dev/null | grep -qE " $c " && echo yes || echo "NO"
done

sec "plain mpv audio test (3 s, no video)"
if command -v mpv >/dev/null 2>&1; then
  mpv --no-video --length=3 --msg-level=all=info "$MOVIE" 2>&1 | grep -iE 'ao|audio|decoder|codec|error|fail' | head -25
else
  echo "mpv CLI not installed (optional: sudo dnf install mpv)"
fi

sec "VAAPI"
(vainfo 2>&1 | head -12) || echo "vainfo not installed (sudo dnf install libva-utils)"

sec "Lumina run (12 s) with mpv messages"
LOG="$(mktemp)"
LUMINA_MPV_OPTS="msg-level=all=v" QT_LOGGING_RULES="*.debug=false" \
  perl -e 'alarm 12; exec @ARGV' "$BIN" "$MOVIE" >"$LOG" 2>&1
grep -iE 'audio|\bao\b|ao/|decoder|hwdec|vaapi|No sound|error|fail|flip' "$LOG" | head -60
echo "--- last lines ---"
tail -15 "$LOG"
echo
echo "App log files: ~/.local/share/mdmakky/Lumina Player/logs (if present)"
ls -la "$HOME/.local/share/mdmakky/Lumina Player/logs" 2>/dev/null | head
