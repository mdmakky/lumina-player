# Lumina Player

**A fast, modern desktop media player built on Qt 6 and libmpv. Smooth seeking and a clean interface are the main goals.**

![Status](https://img.shields.io/badge/status-preview-8175F5)
![License](https://img.shields.io/badge/license-GPL--3.0--or--later-green)
![Platforms](https://img.shields.io/badge/platforms-Linux%20%7C%20macOS-blue)

Lumina plays video and audio with mpv's engine inside a Qt Quick interface: a floating control
overlay, a library, playlists, subtitle and audio tools, and keyboard and mouse bindings you can change.
It was started because seeking in other players stutters on Linux; fast, accurate seeking is the core promise.

> **Preview software.** The current release is `0.3.1-preview`. It is developed and tested on Fedora and macOS;
> expect rough edges. Bug reports are welcome.

## Install

| Platform | Package | Command |
|---|---|---|
| Any Linux | Flatpak | `flatpak install --user ./Lumina-Player_*_Any-Linux-Flatpak.flatpak` |
| Fedora 43, 44, 45 | RPM (one per release) | `sudo dnf install ./Lumina-Player_*_Fedora-<N>.rpm` |
| Ubuntu 24.04, 25.10, 26.04 | DEB (one per release) | `sudo apt install ./Lumina-Player_*_Ubuntu-<N>.deb` |
| macOS (Apple Silicon) | DMG, built on a Mac and attached to a release when ready (may lag the Linux packages) | open it and drag to Applications |
| Windows | not available yet | |

Download from the [Releases page](https://github.com/mdmakky/lumina-player/releases). Pick the
file whose name ends with your system, for example `..._Fedora-45.rpm` or `..._Ubuntu-24.04.deb`. Full instructions, codec setup, what has been tested, and
troubleshooting: **[docs/INSTALL.md](docs/INSTALL.md)**.

## Features

- **Playback:** libmpv engine with hardware decoding where available (VAAPI verified on AMD, VideoToolbox on macOS), accurate seeking, keyframe
  scrubbing while you drag the timeline, hover thumbnails, speed control, A-B loop, chapters, bookmarks
- **Playlists and library:** drag to reorder, shuffle, repeat, M3U import and export; a media library with search,
  tags, thumbnails, duplicate detection, favorites and "continue watching"
- **Subtitles and audio:** embedded and external subtitles, delay controls, style options (including Bengali fonts),
  track menus, 10-band equalizer
- **Picture:** aspect, rotate, crop, brightness / contrast / saturation / gamma / hue presets
- **Network:** open URLs (HTTP, RTSP, RTMP, ...) and page URLs through yt-dlp
- **Interface:** dark, light and OLED themes, mini-player, rebindable shortcuts and mouse actions, Bengali and English UI
- **Linux integration:** MPRIS media controls and keys, screensaver inhibit, volume follows the system volume
- **Local-first:** no account, no telemetry, no cloud. An update check exists but is **off by default** and downloads nothing

See the [User Guide](docs/USER_GUIDE.md) for keyboard shortcuts and every feature.

## Platform support

| Platform | Status |
|---|---|
| Linux (Wayland and X11) | primary target. Fedora is verified on real hardware; Fedora and Ubuntu packages are install- and smoke-tested in CI |
| macOS (Apple Silicon) | works. The DMG is only ad-hoc signed, so Gatekeeper asks you to confirm the first launch |
| Windows | experimental source build only, never run on a Windows machine. Not released |

## Build from source

Requirements: CMake 3.28+, Ninja, a C++20 compiler, Qt 6.4+ (with Quick Controls and SVG), libmpv 2.2+.

```bash
sudo dnf install cmake ninja-build gcc-c++ qt6-qtbase-devel qt6-qtdeclarative-devel \
                 qt6-qtsvg-devel mpv-devel libva-devel        # Fedora; other distros: docs/BUILDING.md
cmake --preset linux-debug
cmake --build --preset linux-debug
./build/linux-debug/lumina [file-or-url]
ctest --preset linux-debug --output-on-failure
```

Details for every platform: [docs/BUILDING.md](docs/BUILDING.md).

## Contributing

Issues and pull requests are welcome. Read [CONTRIBUTING.md](CONTRIBUTING.md) first (architecture rules, how to
test, commit style). Report security problems privately as described in [SECURITY.md](SECURITY.md).

## Documentation

| | |
|---|---|
| [Install guide](docs/INSTALL.md) | packages, codecs, uninstall, troubleshooting |
| [User guide](docs/USER_GUIDE.md) | features and shortcuts |
| [Building](docs/BUILDING.md) | build and run from source |
| [Architecture](docs/ARCHITECTURE.md) | layers and design decisions ([ADRs](docs/TECHNICAL_DECISIONS.md)) |
| [Testing](docs/TESTING.md) | unit tests, self-tests, benchmarks ([seek benchmark](docs/BENCHMARK.md)) |
| [Packaging](docs/PACKAGING.md) | how releases are built |
| [Security review](docs/SECURITY_REVIEW.md) | threat model and hardening |
| [Roadmap](docs/ROADMAP.md) | what is planned |
| [Changelog](CHANGELOG.md) | what changed in each release |

## License

GPL-3.0-or-later. mpv is GPLv2+ and Qt is used under its open-source license; both are compatible.
Third-party notices: [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
