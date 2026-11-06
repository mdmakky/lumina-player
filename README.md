# Lumina Player

**A premium, cross-platform desktop media player.**

Lumina combines mpv-grade playback reliability with a modern, polished, dark-first interface — advanced controls without an overwhelming UI.

> Professional playback capabilities without compromising simplicity, beauty, or performance.

![Status](https://img.shields.io/badge/status-preview-8175F5) ![Platform](https://img.shields.io/badge/platform-Linux-blue) ![License](https://img.shields.io/badge/license-GPL--3.0-green)

## Highlights

- **libmpv playback engine** — hardware-accelerated, format-agnostic, reliable seeking
- **Render-API video output** — GPU frames composited natively into the Qt Quick scene graph (no window embedding hacks)
- **Dark-first premium UI** — custom theme system, cinematic presentation
- **Local-first** — no accounts, no telemetry, no cloud
- Original product — inspired by the best of PotPlayer, mpv, VLC and IINA, cloned from none of them

## Platforms

| Platform | Status |
|---|---|
| Linux (Fedora first, Wayland + X11) | primary target; `.rpm`, `.deb`, Flatpak manifest, build from source |
| macOS (Apple Silicon) | works; ad-hoc signed `.dmg` (Gatekeeper warns) |
| Windows | experimental `.zip`; built by CI, **not yet run on a real Windows machine** |

## Download

Preview builds are on the [Releases page](https://github.com/mdmakky/lumina-player/releases).

- **Windows:** unzip `Lumina-Player-*-windows-x64.zip`, run `lumina.exe` (needs the Microsoft Visual C++ runtime only if Windows reports a missing DLL; the package bundles Qt and libmpv). SmartScreen may warn because the file is unsigned: More info, Run anyway.
- **Fedora:** pick the `.rpm` for **your** Fedora release (`lumina-player-*.fc43|fc44|fc45.x86_64.rpm`), then `sudo dnf install ./lumina-player-*.rpm` (use the terminal: GNOME Software refuses unsigned packages). For E-AC3/DTS audio also enable RPM Fusion and `sudo dnf install libavcodec-freeworld`.
- **Ubuntu:** pick the `.deb` for **your** release (`..._ubuntu24.04_`, `_ubuntu25.10_`, `_ubuntu26.04_`), then `sudo apt install ./lumina-player_*.deb`.
- **Any other Linux (Debian, older Ubuntu/Fedora, Arch, openSUSE ...):** use the Flatpak: `flatpak install --user ./Lumina-Player-*.flatpak`.

| Package | Installs and starts on | Not supported |
|---|---|---|
| `.rpm` | Fedora 43, 44, 45 | other Fedora releases, RHEL/Rocky |
| `.deb` | Ubuntu 24.04, 25.10, 26.04 (see the release page for what CI built) | Ubuntu 22.04 and Debian 12 (Qt older than 6.4) |
| Flatpak | any distro with Flatpak | not hand-tested beyond Fedora 45 |

An RPM/DEB is tied to its release's Qt libraries, so there is one file per release. Each file is installed and its playback self-test run in a clean container of that release by CI before it is attached.
- **macOS:** open the `.dmg`, drag Lumina to Applications, then right-click > Open the first time.

## Building

See [docs/BUILDING.md](docs/BUILDING.md). Quick start (Fedora):

```bash
sudo dnf install qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtquickcontrols2-devel \
                 qt6-qtsvg-devel mpv-devel ninja-build cmake gcc-c++
cmake --preset linux-debug
cmake --build --preset linux-debug
./build/linux-debug/lumina
```

## Documentation

- [Product Master Specification](docs/MASTER_SPEC.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Technical Decision Records](docs/TECHNICAL_DECISIONS.md)
- [Roadmap](docs/ROADMAP.md)
- [Testing](docs/TESTING.md)
- [Engineering context for agents](AGENTS.md)

## License

GPL-3.0-or-later. Third-party notices: [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
