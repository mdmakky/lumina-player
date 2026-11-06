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

## Linux session (2026-11-06) — what was done on Fedora, and what the macOS agent must check
Owner tested by hand on Fedora (GNOME/Wayland); only the items marked "tested" were seen working by the owner.

**Verified on Fedora**
- Seek benchmark, Fight Club 1080p H.264 mkv: hwdec `vaapi-copy` works; exact seek p50 ~265 ms / p95 ~430 ms, keyframe seek p50 ~35 ms,
  0 dropped frames, spam settles ~1.7 s. **VLC comparison not done** (needs `sudo dnf install vlc python3-vlc` from a normal terminal, then `tools/vlc_seekbench.py`).
- **Flatpak now builds** and runs (Wayland and X11 self-test pass, VAAPI works inside the sandbox, E-AC3/DTS decoders compiled in). Manifest fixes:
  KDE runtime 6.8 -> 6.10 (6.8 is gone from Flathub), added a libplacebo module (mpv needs it), mpv `-Dx11=disabled` (xpresent missing; Lumina uses the render API).
  Build: `flatpak run org.flatpak.Builder --user --force-clean --disable-rofiles-fuse <dir> packaging/flatpak/io.github.mdmakky.Lumina.yml`.
  `desktop-file-validate` / `appstreamcli validate --no-net` pass. Still missing: Flathub screenshots; sound/MPRIS/portals inside the Flatpak not hand-tested.
- The stale `build/linux-debug` (configured from another path) had to be recreated; if cmake complains about a different source dir, move the build dir aside and re-run `cmake --preset linux-debug`.

**UI changes made from the owner's hands-on feedback (owner confirmed volume + preview work; the rest was built but not re-confirmed)**
1. All menus (`PopupPanel`) are now `modal: true, dim: false`: clicking outside only closes the menu; before it also toggled pause.
2. Auto-hide: `hideTimer` re-arms while a menu/playlist is open or the chrome is hovered (it used to give up and never hide again after the menu closed).
3. Default subtitle size 55 -> 40 (`SettingsDialog.qml`); existing saved sizes are untouched.
4. **Volume model (Linux):** `PlayerController.systemVolume` (-1 = unknown). When the OS backend reports it, the volume slider, Up/Down keys, mouse wheel and the OS volume keys
   all drive/mirror the **OS sink volume** (`pactl`, 0-100), and the mpv volume is forced to 100 (no double attenuation). When it is -1 (macOS, Windows, no pactl)
   everything falls back to the old mpv volume (0-150). Entry point: `PlayerController::setVolumePercent`. Linux code: `MprisIntegration::watchSystemVolume` (`pactl subscribe`)
   and `setSystemVolume`; new `PlatformIntegration::systemVolumeChanged` signal + `setSystemVolume` virtual (no-op default).
   **macOS agent: `systemVolume` stays -1 there, so verify the slider/keys/wheel still drive mpv volume exactly as before. If you want the same "slider = system volume" on macOS, implement it in `MacIntegration.mm`.**
5. Volume OSD: any volume change shows the chrome and a "Volume N%" toast (`Main.qml volumeOsd`). Mute shows the slider at 0; changing volume while muted unmutes.
6. Mouse wheel / touchpad over the video changes volume: accumulated, 1% per 24 angle units (a mouse notch = 5%), direction follows the gesture via `inverted`. Tune the `24` if too sensitive.
7. `VolumeSlider.qml` redesigned (128 px, thin track that thickens on hover, handle only on hover, % bubble, always-visible `NN%`). No `font.features` (Qt 6.4 CI).
8. Seek-preview card (`TimeSlider.qml` tooltip) restyled: neutral near-black card, one hairline border, no inner frame.
9. Video adjustment popup: preset chips Default / Cinema / Vivid / Night (`VideoAdjustPopup.qml`); values are guesses, tune freely.

**Tests:** `ctest` 11/11 passed on Fedora after items 1-9. `tools/run-ui-test.sh` was **not** run (owner asked not to run big suites); please run it once on macOS, since popups are now modal.
Owner also asked what "Always on top" and "Deinterlace" do: top = window above others (may not work on GNOME Wayland, untested); deinterlace = removes comb lines from old interlaced TV/DVD video, useless for modern movies.

## Open work, in priority order
1. Seek smoothness vs VLC on Fedora (VAAPI is confirmed via `LUMINA_SEEK_BENCH`; VLC numbers still missing).
2. Flatpak: hand-test sound, MPRIS, portals, file dialogs; add Flathub screenshots. (Build itself is done.)
3. Verify the new popup panels, Settings and Bengali (install LinguistTools) on Linux; fix whatever looks wrong there.
4. Windows build (needs real Windows), macOS signing, translations review, crash reporting.
5. Seek-preview thumbnails and library features only checked on macOS test patterns — check with real movies.
