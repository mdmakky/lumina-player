# Packaging

## How a release is built

Run the **Packages** workflow (`gh workflow run Packages -f tag=<release tag>`) for an existing draft release.
It builds and attaches:

| Job | Output | Checked before it is attached |
|---|---|---|
| Fedora 43 / 44 / 45 | `Lumina-Player_<ver>_Fedora-<N>.rpm` | installs in a clean container of that release, then the app starts and plays `tests/data/smoke.mkv` (self-test, virtual display, no audio) |
| Ubuntu 24.04 / 25.10 / 26.04 | `Lumina-Player_<ver>_Ubuntu-<N>.deb` | same, in a clean Ubuntu container; runtime package names are checked first |
| Flatpak | `Lumina-Player_<ver>_Any-Linux-Flatpak.flatpak` | builds from `packaging/flatpak/` (not auto-installed in CI) |
| Windows (opt-in: `-f windows=true`) | `Lumina-Player_<ver>_Windows-experimental.zip` | nothing: never run on real Windows |
| checksums | `SHA256SUMS.txt` | |

RPM and DEB are built per distro release because they link that release's Qt. When a new Fedora or Ubuntu
release appears, add it to the matrix in `packages.yml`; the install and smoke steps show what breaks.
Release file names say which system they are for (`..._Fedora-45.rpm`) so a user can pick without knowing
package-naming rules. The macOS DMG is built on a Mac (`tools/macdeploy.sh`) and uploaded by hand as
`Lumina-Player_<ver>_macOS.dmg`.

Release checklist: CI green on `main` → run **Packages** → all jobs green → read the draft notes
(`docs/releases/`) → publish. Do not publish assets that were not verified.

## Icons
`resources/icons/app/lumina.svg` is the source. `tools/make-icons.sh` regenerates `lumina-{16..1024}.png` and `lumina.icns` (rsvg-convert if present, else AppKit via swift on macOS; `.icns` needs `iconutil`). The 1024 px PNG is git-ignored.

## Linux (Flatpak, primary)
Files: `packaging/flatpak/io.github.mdmakky.Lumina.yml`, `packaging/linux/*.desktop`, `*.metainfo.xml`. CMake installs binary, desktop file, metainfo and hicolor icons (`cmake --install`); `cpack -G TGZ` makes a tarball.

Fedora, no RPM needed:
```bash
sudo dnf install flatpak flatpak-builder
flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install --user flathub org.kde.Platform//6.10 org.kde.Sdk//6.10 org.flatpak.Builder
flatpak run org.flatpak.Builder --user --install --force-clean build-flatpak packaging/flatpak/io.github.mdmakky.Lumina.yml
flatpak run io.github.mdmakky.Lumina /path/to/video.mkv
```

## macOS
```bash
tools/macdeploy.sh      # -> build/mac-bundle/bin/lumina.app and build/mac-bundle/Lumina-Player.dmg
```
Normal dev builds leave `LUMINA_MAC_BUNDLE` OFF (plain `build/mac/bin/lumina`). Turn it ON to get `bin/lumina.app` (Info.plist from `packaging/macos/Info.plist.in`, document types for Finder "Open With").

## Release packages (CI)
`.github/workflows/packages.yml` (manual: `gh workflow run Packages -f tag=v0.3.1-preview`) builds the Windows zip (MSYS2 MinGW64 + the shinchiro libmpv SDK + windeployqt), the `.deb` (Ubuntu 24.04) and the `.rpm` (Fedora container) and attaches them to the given release. The macOS dmg is built locally with `tools/macdeploy.sh` and uploaded with `gh release upload`.

## Other Linux package recipes
- RPM: `packaging/rpm/lumina.spec` (`Requires: mpv-libs`, `Recommends: libavcodec-freeworld`). Build with `rpmbuild -ba` after `git archive` of the tag; check with `rpmlint`.
- DEB: `packaging/deb/debian/` (CI builds it; copy or symlink to `debian/` in the source root, then `dpkg-buildpackage -us -uc`). Verify Qt 6 QML module package names on the target Debian/Ubuntu.
- AppImage: `packaging/appimage/build-appimage.sh` (linuxdeploy + Qt plugin; run on an old-glibc host; GPU/VAAPI libraries are excluded on purpose).
- macOS signing and notarization steps: `packaging/macos/README.md`.

## Release checklist
1. `ctest`, `tools/run-ui-test.sh`, playback self-test on each platform (`docs/QA_REPORT.md`).
2. `tools/make-sbom.sh docs/sbom-<version>.json` on the release build; ship `THIRD_PARTY_NOTICES.md`.
3. Bump version in `CMakeLists.txt`, `CHANGELOG.md`, metainfo `<release>`, spec and debian changelog; tag `vX.Y.Z`.
4. Build packages, install on a clean machine, run the manual matrix.

## Verification status
Verified on macOS: both bundle ON/OFF build; bundle plist/icns valid; `macdeploy.sh` runs and produces an 83 MB dmg, main binary has no `/opt/homebrew` references left.

**Unverified:**
- Flatpak manifest: built and run on Fedora 45 (KDE 6.10 runtime, Wayland + X11 self-test pass, VAAPI works, E-AC3/DTS decoders present). Not checked: sound by ear, Flathub submission lint.
- `.desktop` / metainfo: `desktop-file-validate` and `appstreamcli validate --no-net` pass (Fedora); screenshots still missing (Flathub requires them).
- macOS: bundled app launch/playback not tested; macdeployqt prints rpath errors for QML plugins whose Qt modules are not installed (VirtualKeyboard, Multimedia); libmpv dylib rewriting is best-effort; signing is ad-hoc only (no Developer ID/notarization, Gatekeeper warns); Finder "Open With" not exercised.
- Windows packaging: see docs/WINDOWS.md (experimental, never built on Windows).

## Codecs and dependencies (what gets installed for the user)

- **Flatpak (primary Linux package):** FFmpeg (all native decoders incl. E-AC3/DTS) and libmpv are built into the app,
  so codecs work out of the box; nothing for the user to install.
- **From source on Fedora:** run `tools/install-deps-fedora.sh` (build deps + RPM Fusion `libavcodec-freeworld`).
  Fedora's default `ffmpeg-free` lacks E-AC3/DTS/H.264 software decoders; Lumina shows a banner with the fix command if a
  file's audio can't be decoded.
- **RPM (if we ship one):** `Requires: mpv-libs`, `Recommends: libavcodec-freeworld` (a hard Require on RPM Fusion is not
  allowed by Fedora policy).
- **macOS / Windows bundles:** libmpv + FFmpeg are bundled with the app.
