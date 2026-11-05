# AGENTS.md — Lumina Player Engineering Context

**Read this file first.** It is the single source of truth for any agent (or human) continuing work on this project. Keep it synchronized with reality at all times — a stale AGENTS.md is worse than none.

---

## 1. What This Project Is

**Lumina Player** — a next-generation, cross-platform desktop media player written in C++20 with Qt 6 / QML as the UI layer and libmpv as the playback engine.

- **Owner / GitHub account:** `mdmakky` (git user: mdmakky <makky.cse@gmail.com>)
- **Primary dev platform:** Linux (Fedora 45 first), targets Linux/Windows/macOS
- **Full product spec:** [`docs/MASTER_SPEC.md`](docs/MASTER_SPEC.md) — the complete PRD + architecture + UX + execution plan. It is authoritative for product intent; where this file and the spec disagree on *intent*, the spec wins; where they disagree on *current reality*, this file wins.
- **Product philosophy:** "Professional playback capabilities without compromising simplicity, beauty, or performance." Original product identity — not a PotPlayer clone.

## 2. Technology Stack (decided — do not churn)

| Layer | Choice |
|---|---|
| Language | C++20 |
| UI | Qt 6 + Qt Quick + QML (custom controls, "Basic" style) |
| Playback | libmpv embedded via client API + **render API** (OpenGL), NOT `wid` embedding, NOT external mpv process |
| Media probing | FFmpeg via mpv; direct FFmpeg libs only when mpv can't do it |
| Database | SQLite (Phase: Milestone 7) |
| Build | CMake ≥ 3.28 + Ninja + CMakePresets + CTest |
| Testing | Qt Test (unit), later GoogleTest |
| License | GPL-3.0 (mpv is GPLv2+; Qt open source — compatible) |

## 3. Architecture (layered — enforce boundaries)

```
app/            Application bootstrap, ApplicationContext
src/core/       Presentation-agnostic app state: PlayerController, playback queue
src/engine/     Media engine: MpvEngine (client API), MpvVideoItem (render API/QML)
src/media/      MediaProbe, metadata, thumbnails (Milestone 7)
src/services/   Subtitle/playlist services (Milestones 4, 7)
src/storage/    SQLite repositories (Milestone 7)
src/platform/   Per-OS adapters (Milestone 8) — never put OS code in shared layers
src/utilities/  Logger, time formatting, small helpers
ui/             QML: Main.qml, Theme/, Components/, Player/, Settings/
```

Rules:
- **UI never contains playback logic.** QML talks to PlayerController only.
- **PlayerController never talks to mpv directly** — only through the MpvEngine interface.
- **MpvVideoItem** owns the mpv render context (GL) and is the *only* class touching both mpv render and Qt scene graph.
- Colors/spacing come from `ui/Theme/Theme.qml` — never hardcode colors in components.

## 4. Current Status (update after every milestone)

- **Milestone 1 — Foundation:** ✅ DONE (repo, CMake, Qt bootstrap, main window, theme system, logging, CI)
- **Milestone 2 — Playback Engine:** ✅ DONE AND VERIFIED — automated playback self-test (`LUMINA_PLAYBACK_TEST=1`) passes: load → play → accurate seek → pause → resume, mpv render context created on OpenGL scene graph. GitHub repo: `mdmakky/lumina-player` (main).
- **Milestone 3 — Core UX:** ✅ essentially done (floating overlay controls, timeline + chapters, playlist panel, window/volume persistence, single-instance; remaining: drag-reorder, m3u save/load)
- **Milestone 4 — Subtitle & Audio:** 🚧 track menu, external subs, delays done; subtitle style prefs pending
- **Milestone 5 — Performance:** 🚧 stats overlay (I), seek/cache tuning, `LUMINA_SEEK_STRESS=1` test done; hwdec ladder UI pending
- **Milestones 6–9:** ⬜ not started

### Continuation on macOS (dev machine moved)

The project now continues development on macOS. To set up there:

```bash
git clone git@github.com:mdmakky/lumina-player.git
cd lumina-player
brew install cmake ninja qt libmpv
cmake --preset linux-debug   # preset works; rename/copy to a mac preset if desired
cmake --build --preset linux-debug
LUMINA_PLAYBACK_TEST=1 ./build/linux-debug/lumina <some-video-file>
```

On macOS the presets are not used: `cmake -S . -B build/mac -G Ninja -DCMAKE_PREFIX_PATH=$(brew --prefix qt)`; binary is `build/mac/bin/lumina` (case-insensitive FS clash with the `Lumina/` QML dir).

macOS notes: mpv's OpenGL render API works with Qt's GL scene graph on macOS; consider `QQuickWindow::setGraphicsApi(OpenGLRaster)` if the default Metal path interferes — do NOT switch the app to Metal rendering without re-doing ADR-002. The playback self-test harness is the fastest way to verify the engine after any environment change.

### Known-good baseline (this commit state)

- Fedora 45: GCC 16.2, CMake 4.3, Qt 6.11.2, libmpv 2.5.0 (mpv 0.41), Ninja, Wayland, AMD `amdgpu` (VAAPI-capable).
- GitHub Actions ubuntu-latest: Qt 6.4.2 — this is why `find_package(Qt6 6.4)` is the minimum; do not raise it above 6.4 without dropping CI.
- `ffmpeg-devel` conflicts with Fedora's free ffmpeg libs — **do not install it**; mpv-devel provides everything.
- Self-test verdict is strict: requires duration > 5 s, position ≥ 10 s after seek, no engine error. Exit 0 = pass.

## 5. Build & Test

```bash
# configure (first time)
cmake --preset linux-debug
# build
cmake --build --preset linux-debug
# run
./build/linux-debug/lumina
# unit tests
ctest --preset linux-debug
```

Presets live in `CMakePresets.json`. If a build fails, fix it before committing. Never commit untested code.

## 6. Engineering Rules (non-negotiable, from spec §19)

1. Never block the UI thread with decoding or long-running operations.
2. Never duplicate engine capabilities (mpv already does deinterlace, EQ, subtitle rendering…).
3. Never mix UI state with media-engine state.
4. Never hardcode OS-specific behavior in shared modules.
5. Never ignore memory leaks, unbounded queues, or silently swallow playback errors.
6. Never claim a feature works without building and testing it.
7. Never hardcode colors outside Theme.qml.
8. Playback must keep working if the database is unavailable (library is optional).

## 7. Workflow Conventions

- **Small, logical commits.** Format: `type: short imperative summary` — types: `feat:`, `fix:`, `docs:`, `build:`, `test:`, `refactor:`, `chore:`.
- One logical change per commit; docs and code they describe go together when docs explain that change.
- Update the "Current Status" section here and `docs/ROADMAP.md` when a milestone item completes.
- Remote: `origin` → `git@github.com:mdmakky/lumina-player.git` (or HTTPS with gh). Push to `main` only after build+tests pass.
- CI: `.github/workflows/ci.yml` builds on ubuntu-latest with Qt 6 + libmpv-dev and runs CTest.

## 8. Key Design Decisions Already Made (ADR summary)

See `docs/TECHNICAL_DECISIONS.md` for the full record. Headlines:
- **Render API over `wid` embedding** — hardware-accelerated, compositable, works on Wayland.
- **QQuickFramebufferObject** as the bridge between mpv GL rendering and Qt Quick — mpv renders into Qt's FBO on the render thread; GUI thread is never blocked.
- **Force OpenGL scene-graph API** on desktop (Qt 6 default RHI varies; GL is the only path mpv's OpenGL render API supports).
- **"Basic" Qt Quick Controls style + custom theme** — full visual control for the premium dark-first look (spec §9 tokens).

## 9. Where Things Are

- Master spec: `docs/MASTER_SPEC.md` · Architecture: `docs/ARCHITECTURE.md` · ADRs: `docs/TECHNICAL_DECISIONS.md` · Building: `docs/BUILDING.md` · Roadmap: `docs/ROADMAP.md` · Test strategy: `docs/TESTING.md`
- mpv render bridge: `src/engine/MpvVideoItem.*` · mpv client wrapper: `src/engine/MpvEngine.*` · QML controller: `src/core/PlayerController.*`
- Theme tokens (spec §9): `ui/Theme/Theme.qml`

## 10. Next Steps (pick up here)

1. Milestone 3: playlist panel + drag & drop + fullscreen polish + idle-autohide controls.
2. Milestone 4: subtitle track management UI (mpv handles rendering; we manage track lists/sync).
3. Milestone 5: hardware acceleration manager (VAAPI first — AMD VAAPI present on dev machine), playback statistics OSD.
4. Add integration test media generation (ffmpeg testsrc) — see `docs/TESTING.md`.
