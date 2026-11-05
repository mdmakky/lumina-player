# Handoff — state at the end of the macOS development session (2026-11-05)

Read `AGENTS.md` first (architecture, rules, test switches). This file is the *session memory*: what happened, what was
learned on the owner's Fedora machine, what is still open, and how the owner likes to work.

## Who / how to work with the owner
- Owner: `mdmakky` (GitHub). Writes **Bengali in Latin letters mixed with English** ("kaj korche", "sound ashe").
  Reply in the same style (short, direct). Primary target is **Linux (Fedora 45, GNOME/Wayland, AMD Ryzen APU)**.
- Wants: fast progress, fewer words, **do not re-run the full test suite repeatedly** ("ek test bar bar kore somoy nosto
  koris na"). Run the one check that proves the fix, then move on.
- Wants a professional, polished UI (they judged early designs harshly); screenshots they send are the truth.
- Their terminal is sometimes inside a sandboxed IDE (ZCode) where `sudo` is blocked ("no new privileges"): sudo commands
  must be run from a normal terminal. They also sometimes paste placeholder paths literally — give real paths.
- Their pain that started the project: VLC freezes/drops frames on seek on Fedora. Smooth seeking is the core promise.

## Repo state (all pushed to `mdmakky/lumina-player`, main; version 0.3.0)
Done and tested on macOS (unit tests 11/11, in-app UI test ~180 checks, CI green on Linux Qt 6.4 + macOS):
playback engine (mpv render API), floating-overlay UI, playlist (drag reorder/shuffle/repeat/M3U), chapters, A-B loop,
bookmarks, equalizer, audio/subtitle track panel (+delays), screenshots, mini-player, Dark/Light/OLED themes, reduced
motion, Settings (10 pages + search + reset), rebindable shortcuts, SQLite history + Continue watching + favorites,
optional media library, per-file settings, network streams (Open URL, yt-dlp), seek-bar hover thumbnails, seek coalescing,
Bengali translation (live language switch), MPRIS + screensaver inhibit (Linux), packaging files.

Test switches: see AGENTS.md ("Self-test env switches"). On macOS the binary is `build/mac/bin/lumina`; on Linux it is
`build/linux-debug/lumina`. UI test: `tools/run-ui-test.sh /tmp/long.mp4` (forces focus; needs a >=40 s file).

## What the owner's Fedora machine showed (real hardware — the only Linux evidence we have)
- App builds and runs (Qt 6.11.2, libmpv 2.5/mpv 0.41, Wayland). Video, subtitles, resume, MPRIS registration and screensaver
  inhibit work. **Sound works** after installing RPM Fusion `libavcodec-freeworld` (movie audio was E-AC3; Fedora's
  `ffmpeg-free` lacks E-AC3/DTS). The app now shows a "No sound ... run: sudo dnf install libavcodec-freeworld" banner.
- VAAPI: first run fell back to software H.264 (`No support for codec h264 profile 100`) because only
  `libva-intel-media-driver` was installed. After `mesa-va-drivers-freeworld` `vainfo` lists H264 High VLD on radeonsi.
  **Not yet confirmed**: that Lumina's stats overlay (`I`) now shows `vaapi`.
- Not yet reported by the owner: seek smoothness vs VLC (use `LUMINA_SEEK_BENCH=1`, docs/BENCHMARK.md), MPRIS widget,
  Bengali (they have not installed `qt6-linguist qt6-qttools-devel`, so the build has no translations), panel redesign on Linux.
- Helper scripts: `tools/diagnose-linux.sh <movie>` (asks for real movie path), `tools/install-deps-fedora.sh`.

## Decisions and gotchas learned (do not re-learn them)
- macOS only: ao `coreaudio,avfoundation` (avfoundation alone delays speed changes ~4 s; coreaudio alone cannot open 48 kHz
  devices), `audio-channels=stereo,5.1,7.1`, GL 3.3 core profile, `hwdec=videotoolbox-copy` (screenshots need it). All guarded
  by `Q_OS_MACOS`. Linux uses mpv defaults + `hwdec=vaapi,nvdec,auto-safe`.
- Video orientation: `flip_y=0` (screenshots on macOS showed upside-down text with 1). **Unverified on Linux** — the owner's
  movie looked upright on Fedora, so it is probably right. `LUMINA_FLIP_Y=1` flips it if a platform differs.
- CI uses Qt 6.4: no `font.features`, no JS reserved words as identifiers (`goto` broke the QML compiler), use
  `QStringLiteral`/`QLatin1String` in XML attribute lookups. Windows CI job is manual-only (MSYS2 package names unverified).
- Self-tests/UI tests use isolated QSettings/DB (`Lumina Player UITest`); they once polluted the real settings.
- Flatpak manifest (`packaging/flatpak`) builds FFmpeg from source (all decoders) but has **never been built**.
- Not done on purpose (needs the owner's accounts): OpenSubtitles download, code signing/notarization, auto-update.

## Open work, in priority order
1. Confirm hardware decode on Fedora (`I` overlay) and seek smoothness vs VLC; tune from `LUMINA_SEEK_BENCH` numbers.
2. Build the Flatpak on Fedora (`flatpak-builder`), fix the manifest.
3. Verify the new popup panels, Settings and Bengali (install LinguistTools) on Linux; fix whatever looks wrong there.
4. Windows build (needs real Windows), macOS signing, translations review, crash reporting.
5. Seek-preview thumbnails and library features only checked on macOS test patterns — check with real movies.
