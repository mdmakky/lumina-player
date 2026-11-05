# Lumina Player

**A premium, cross-platform desktop media player.**

Lumina combines mpv-grade playback reliability with a modern, polished, dark-first interface — advanced controls without an overwhelming UI.

> Professional playback capabilities without compromising simplicity, beauty, or performance.

![Status](https://img.shields.io/badge/status-early_development-8175F5) ![Platform](https://img.shields.io/badge/platform-Linux-blue) ![License](https://img.shields.io/badge/license-GPL--3.0-green)

## Highlights

- **libmpv playback engine** — hardware-accelerated, format-agnostic, reliable seeking
- **Render-API video output** — GPU frames composited natively into the Qt Quick scene graph (no window embedding hacks)
- **Dark-first premium UI** — custom theme system, cinematic presentation
- **Local-first** — no accounts, no telemetry, no cloud
- Original product — inspired by the best of PotPlayer, mpv, VLC and IINA, cloned from none of them

## Platforms

| Platform | Status |
|---|---|
| Linux (Fedora first, Wayland + X11) | ✅ primary development |
| Windows | planned |
| macOS | planned |

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
