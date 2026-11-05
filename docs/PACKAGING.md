# Packaging

## Icons
`resources/icons/app/lumina.svg` is the source. `tools/make-icons.sh` regenerates `lumina-{16..1024}.png` and `lumina.icns` (rsvg-convert if present, else AppKit via swift on macOS; `.icns` needs `iconutil`). The 1024 px PNG is git-ignored.

## Linux (Flatpak, primary)
Files: `packaging/flatpak/io.github.mdmakky.Lumina.yml`, `packaging/linux/*.desktop`, `*.metainfo.xml`. CMake installs binary, desktop file, metainfo and hicolor icons (`cmake --install`); `cpack -G TGZ` makes a tarball.

Fedora, no RPM needed:
```bash
sudo dnf install flatpak flatpak-builder
flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install --user flathub org.kde.Platform//6.8 org.kde.Sdk//6.8
flatpak-builder --user --install --force-clean build-flatpak packaging/flatpak/io.github.mdmakky.Lumina.yml
flatpak run io.github.mdmakky.Lumina /path/to/video.mkv
```

## macOS
```bash
tools/macdeploy.sh      # -> build/mac-bundle/bin/lumina.app and build/mac-bundle/Lumina-Player.dmg
```
Normal dev builds leave `LUMINA_MAC_BUNDLE` OFF (plain `build/mac/bin/lumina`). Turn it ON to get `bin/lumina.app` (Info.plist from `packaging/macos/Info.plist.in`, document types for Finder "Open With").

## Verification status
Verified on macOS: both bundle ON/OFF build; bundle plist/icns valid; `macdeploy.sh` runs and produces an 83 MB dmg, main binary has no `/opt/homebrew` references left.

**Unverified:**
- Flatpak manifest: NOT built (YAML syntax only); mpv/FFmpeg tags, runtime version and module options are guesses; libass tarball sha256 unchecked.
- `.desktop` / metainfo: `desktop-file-validate` and `appstreamcli validate` not available here (XML well-formedness only); screenshots missing (Flathub requires them).
- macOS: bundled app launch/playback not tested; macdeployqt prints rpath errors for QML plugins whose Qt modules are not installed (VirtualKeyboard, Multimedia); libmpv dylib rewriting is best-effort; signing is ad-hoc only (no Developer ID/notarization, Gatekeeper warns); Finder "Open With" not exercised.
- Windows packaging: see docs/WINDOWS.md (experimental, never built on Windows).
