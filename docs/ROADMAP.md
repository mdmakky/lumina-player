# Roadmap

Source of truth for milestone status. Spec §18 defines the full milestone plan; this file tracks *actual* completion with evidence.

## Milestone 1 — Foundation ✅

- [x] Repository bootstrap, git history
- [x] CMake + Ninja + CMakePresets configuration
- [x] Qt application bootstrap, main window
- [x] Centralized theme system (`ui/Theme/Theme.qml`)
- [x] Core module boundaries (core/engine/utilities)
- [x] Categorized logging (`src/utilities/Logger.h`)
- [x] Basic CI (GitHub Actions: build + ctest on ubuntu-latest)
- [x] Unit test harness (Qt Test via CTest)

**Acceptance:** builds and opens on Fedora 45 (Wayland). ✅ verified

## Milestone 2 — Playback Engine ✅

- [x] libmpv integration via client API (`MpvEngine`)
- [x] Video rendering via render API into Qt Quick scene (`MpvVideoItem`)
- [x] Audio playback
- [x] Play / pause / stop, file opening (CLI arg + dialog + drag-drop)
- [x] Seeking (fast + accurate), playback state tracking
- [x] Volume, speed, mute controls
- [x] Error reporting surface to UI (media-error banner)
- [x] Track listing (audio/subtitle/video tracks exposed to UI)
- [x] OSD-less clean integration (mpv OSD disabled, UI renders all controls)

**Acceptance:** common video/audio files play reliably. ✅ build + launch verified; playback validated with generated test media

## Milestone 3 — Core UX 🚧 (in progress)

- [x] Modern player controls overlay with idle auto-hide
- [x] Seek timeline with buffered preview + hover time tooltip
- [x] Fullscreen (F), always-on-top (T), borderless
- [x] Keyboard shortcuts (spec §10 defaults)
- [ ] Playlist panel (queue, reorder, save/load, search)
- [ ] Drag & drop playlist reordering
- [x] Chapters UI (timeline markers, chapter menu, PgUp/PgDn)
- [ ] Multiple instance / single-instance behavior decision
- [x] Window state persistence (geometry, volume, mute, recent files)

## Milestone 4 — Subtitle & Audio ⬜

- [ ] Subtitle track management UI (mpv renders; we manage)
- [ ] External subtitle loading + auto-discovery
- [ ] Subtitle delay / sync controls
- [ ] Audio track selection UI, audio delay
- [ ] Subtitle style preferences (persisted)

## Milestone 5 — Performance ⬜

- [ ] Hardware acceleration manager (VAAPI → NVDEC → software fallback ladder)
- [ ] Decoder selection UI (auto / hw-preferred / sw / manual)
- [ ] Seek optimization + rapid-seek stress testing
- [ ] Statistics overlay (decoder, renderer, dropped frames, FPS, bitrate)
- [ ] Benchmark suite vs mpv/VLC on identical hardware

## Milestone 6 — Advanced Features ⬜

A-B repeat, bookmarks, video processing (brightness/contrast/…), equalizer, playback profiles, advanced settings.

## Milestone 7 — Media Library ⬜

SQLite persistence, folder scanning, watch history, continue-watching, thumbnails, search/favorites. Library remains optional (spec rule 12).

## Milestone 8 — Cross-platform ⬜

Windows (D3D11VA) and macOS (VideoToolbox) builds, platform adapters, packaging (Flatpak/RPM/AppImage/DEB, installer, DMG).

## Milestone 9 — Release Preparation ⬜

Security review, license audit + SBOM, QA report, performance report, user docs, packages.
