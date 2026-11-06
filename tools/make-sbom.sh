#!/bin/sh
# Generates a CycloneDX 1.5 SBOM (JSON) for Lumina Player. No network.
# Versions come from pkg-config, then rpm/dpkg/brew when available; "unknown" otherwise.
# Usage: tools/make-sbom.sh [output.json]   (default: stdout)
cd "$(dirname "$0")/.."
OUT="${1:-/dev/stdout}"
VER=$(sed -n 's/^project(Lumina VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)

ver() { # $1 = pkg-config name, $2 = rpm/deb name, $3 = brew name
  v=$(pkg-config --modversion "$1" 2>/dev/null)
  [ -n "$v" ] && { echo "$v"; return; }
  v=$(rpm -q --qf '%{VERSION}' "$2" 2>/dev/null)
  case "$v" in ""|*"not installed"*) ;; *) echo "$v"; return;; esac
  v=$(dpkg-query -W -f='${Version}' "$2" 2>/dev/null)
  [ -n "$v" ] && { echo "$v"; return; }
  v=$(brew list --versions "$3" 2>/dev/null | awk '{print $2}')
  [ -n "$v" ] && { echo "$v"; return; }
  echo unknown
}

first=1
comp() { # name version license
  [ $first = 1 ] || printf ',\n'
  first=0
  printf '    {"type":"library","name":"%s","version":"%s","licenses":[{"expression":"%s"}],"purl":"pkg:generic/%s@%s"}' "$1" "$2" "$3" "$1" "$2"
}

{
  printf '{\n  "bomFormat": "CycloneDX", "specVersion": "1.5", "version": 1,\n'
  printf '  "metadata": {"timestamp": "%s", "component": {"type": "application", "name": "lumina-player", "version": "%s", "licenses": [{"license": {"id": "GPL-3.0-or-later"}}]}},\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$VER"
  printf '  "components": [\n'
  comp qt6 "$(ver Qt6Core qt6-qtbase qt)" "LGPL-3.0-only OR GPL-3.0-only"
  comp libmpv "$(ver mpv mpv-libs mpv)" "GPL-2.0-or-later OR LGPL-2.1-or-later"
  comp ffmpeg-libavcodec "$(ver libavcodec libavcodec-free ffmpeg)" "LGPL-2.1-or-later OR GPL-2.0-or-later"
  comp libass "$(ver libass libass libass)" "ISC"
  comp libplacebo "$(ver libplacebo libplacebo libplacebo)" "LGPL-2.1-or-later"
  comp libva "$(ver libva libva libva)" "MIT"
  comp sqlite3 "$(ver sqlite3 sqlite sqlite)" "blessing"
  printf '\n  ]\n}\n'
} > "$OUT"
