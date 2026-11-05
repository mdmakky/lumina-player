# Architecture

## Overview

Lumina Player is a layered desktop application. Each layer talks only to the layer below it through deliberate interfaces. The playback engine (libmpv) is isolated behind two boundary types: a **client API wrapper** (`MpvEngine`) for control/state, and a **render API bridge** (`MpvVideoItem`) for video presentation.

```
┌──────────────────────────────────────────────────────┐
│  Presentation (QML / Qt Quick)                        │
│  Main.qml · Controls overlay · Theme · Dialogs        │
└──────────────┬───────────────────────────────────────┘
               │ QML properties/signals (data only)
┌──────────────▼───────────────────────────────────────┐
│  Application Core                                     │
│  PlayerController · PlaybackQueue · EventDispatcher   │
└──────────────┬───────────────────────────────────────┘
               │ C++ API (no mpv types leak)
┌──────────────▼───────────────────────────────────────┐
│  Media Engine                                         │
│  MpvEngine (client API) · MpvVideoItem (render API)   │
└──────────────┬───────────────────────────────────────┘
               │ mpv_handle / mpv_render_context
┌──────────────▼───────────────────────────────────────┐
│  libmpv (+ FFmpeg, libplacebo, VA-API/NVDEC/VT)       │
└──────────────────────────────────────────────────────┘
```

## Threading model

| Thread | Responsibilities |
|---|---|
| GUI thread | QML scene, PlayerController, property mutation, mpv commands |
| mpv internal threads | decoding, audio, demuxing (managed by libmpv) |
| Qt Quick render thread | `MpvVideoItem::Renderer::render()` — calls `mpv_render_context_render()` with Qt's FBO bound |

Rules:
- mpv property observation events are drained on the GUI thread via `mpv_set_wakeup_callback` → queued `QMetaObject::invokeMethod`. No mpv API calls from foreign threads except render.
- `mpv_render_context_render()` runs on the Qt render thread with a current GL context — this is explicitly supported by libmpv.
- Seek, load, and stop are asynchronous operations; the UI reflects state from observed properties, never from command return values (except for immediate errors).

## Video presentation

`MpvVideoItem` subclasses `QQuickFramebufferObject`:

1. The item creates an `mpv_render_context` (OpenGL API) after the scene graph is initialized, using the window's `QOpenGLContext` for `get_proc_address`.
2. The update callback (`MPV_RENDER_PARAM_UPDATE_CB`) schedules `QQuickFramebufferObject::update()` on the GUI thread.
3. `Renderer::render()` renders the latest frame into Qt's framebuffer object. `mirrorVertically` is `true` because Qt FBOs and mpv have opposite vertical origins.
4. The scene-graph API is forced to OpenGL (`QQuickWindow::setGraphicsApi`) — mpv's OpenGL render API requires it.

This avoids native window-handle (`wid`) embedding entirely: video is a real Qt Quick item, so overlays, animations, and rounded corners work on Wayland and X11 alike.

## Future layers (not yet implemented)

- **Media Services**: probing, thumbnails, subtitle discovery — separate worker threads, results via signals.
- **Persistence**: SQLite via `DatabaseManager` + repository interfaces; playback must not depend on it.
- **Platform Abstraction**: `src/platform/<os>/` adapters for file dialogs, media keys, power management, packaging.
- **Plugin system**: permission-documented, sandboxed-by-default (spec §14).
