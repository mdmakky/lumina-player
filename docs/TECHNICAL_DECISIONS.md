# Technical Decision Records

Append-only. When a decision is revisited, supersede the entry rather than deleting it.

## ADR-001: libmpv embedded via client + render APIs

**Status:** Accepted · **Date:** 2026-11-05

libmpv provides the playback stack (demuxing, decoding incl. hardware acceleration, subtitle rendering, audio). We embed it with the client API for control and the OpenGL render API for presentation.

Rejected alternatives:
- Launching mpv as a subprocess: spec §5 explicitly forbids as primary architecture; IPC overhead, no clean QML integration.
- Native window-handle (`wid`) embedding: breaks on Wayland compositor restrictions, video cannot be composited with Qt Quick content, limited overlay support. May be evaluated later only as a fallback.

## ADR-002: QQuickFramebufferObject as the mpv↔Qt Quick bridge

**Status:** Accepted · **Date:** 2026-11-05

`QQuickFramebufferObject` is purpose-built for external GL renderers: the renderer object lives on the Qt render thread with a current GL context, and Qt manages FBO lifecycle/resizing. mpv renders into Qt's FBO; `mirrorVertically = true` corrects the vertical origin mismatch.

Alternatives: raw `QQuickItem` + manual FBO blit (more code, same result, more failure modes); `QSGSimpleTextureNode` with mpv-rendered textures (requires frame upload synchronization we don't need).

Consequence: scene-graph backend must be OpenGL. Enforced in `app/main.cpp` via `QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL)`.

## ADR-003: Qt Quick Controls "Basic" style + custom Theme singleton

**Status:** Accepted · **Date:** 2026-11-05

Premium dark-first design (spec §9) requires full control over look and feel. Material/Fusion styles inject their own visual language. We use the unstyled "Basic" style and our own `Theme.qml` singleton with the spec's design tokens (Background `#101114`, Accent `#8175F5`, …). All components reference the theme; hardcoded colors are forbidden.

## ADR-004: Qt Test for unit testing, CTest as harness

**Status:** Accepted · **Date:** 2026-11-05

Qt Test integrates with CTest without extra dependencies and covers our QML-adjacent C++ classes. GoogleTest may be adopted later for pure-C++ layers if needed.

## ADR-005: GPL-3.0-or-later licensing

**Status:** Accepted · **Date:** 2026-11-05

mpv is GPLv2+; Qt open-source editions are GPLv3-compatible. GPL-3.0 satisfies license compatibility for a source-open product. If closed-source distribution ever becomes a requirement, a full dependency re-audit (spec §17) is mandatory *before* architectural changes.

## ADR-006: No ffmpeg-devel at build time on Fedora 45

**Status:** Accepted · **Date:** 2026-11-05

rpmfusion's `ffmpeg-devel` conflicts with Fedora's `lib*-free` packages. The playback stack needs FFmpeg only *through* mpv, whose headers/ABI we consume via `mpv-devel`. Direct FFmpeg API usage (thumbnails, metadata probing) is deferred; when needed, evaluate building against a vendored FFmpeg or using libav* headers shipped via another route.
