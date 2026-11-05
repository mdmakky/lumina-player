#!/bin/sh
# Builds Lumina.app (LUMINA_MAC_BUNDLE=ON), bundles Qt via macdeployqt, then
# copies libmpv and its Homebrew dylib dependency tree into Contents/Frameworks
# Output: build/mac-bundle/bin/lumina.app and Lumina-Player.dmg
set -e
cd "$(dirname "$0")/.."
export PATH=/opt/homebrew/bin:$PATH
B=build/mac-bundle
cmake -S . -B $B -G Ninja -DCMAKE_BUILD_TYPE=Release -DLUMINA_MAC_BUNDLE=ON \
      -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build $B
APP=$B/bin/lumina.app
"$(brew --prefix qt)/bin/macdeployqt" "$APP" -qmldir=ui
# macdeployqt skips non-Qt dylibs it can't resolve via rpath; vendor libmpv tree.
FW=$APP/Contents/Frameworks; BIN=$APP/Contents/MacOS/lumina
mkdir -p "$FW"
queue="$(otool -L "$BIN" | awk '/libmpv/{print $1}')"
seen=""
while [ -n "$queue" ]; do
  set -- $queue; lib=$1; shift; queue="$*"
  case " $seen " in *" $lib "*) continue;; esac
  seen="$seen $lib"
  case "$lib" in /opt/homebrew/*|/usr/local/*) ;; *) continue;; esac
  real=$(python3 -c 'import os,sys;print(os.path.realpath(sys.argv[1]))' "$lib")
  dst="$FW/$(basename "$lib")"
  [ -e "$dst" ] || cp "$real" "$dst"; chmod u+w "$dst"
  install_name_tool -id "@executable_path/../Frameworks/$(basename "$lib")" "$dst"
  for dep in $(otool -L "$dst" | awk 'NR>1{print $1}' | grep -E '^/(opt/homebrew|usr/local)'); do
    queue="$queue $dep"
    install_name_tool -change "$dep" "@executable_path/../Frameworks/$(basename "$dep")" "$dst"
  done
  install_name_tool -change "$lib" "@executable_path/../Frameworks/$(basename "$lib")" "$BIN"
done
codesign --force --deep --sign - "$APP"   # ad-hoc; Developer ID signing + notarization still TODO
hdiutil create -volname "Lumina Player" -srcfolder "$APP" -ov -format UDZO "$B/Lumina-Player.dmg"
echo "Done: $APP  $B/Lumina-Player.dmg"
