# Lumina Player User Guide

Lumina Player is a fast desktop media player (Qt 6 + libmpv). Smooth seeking and a clean interface are the main goals.

## 1. Install (ইনস্টল)

Download the package for your system from the [Releases page](https://github.com/mdmakky/lumina-player/releases)
and follow **[INSTALL.md](INSTALL.md)** (Flatpak for any Linux, RPM for Fedora 43/44/45, DEB for Ubuntu
24.04/25.10/26.04, DMG for macOS). It also covers codecs, uninstalling and troubleshooting.

To build from source instead, see [BUILDING.md](BUILDING.md). Install `qt6-linguist qt6-qttools-devel` before
configuring if you want the Bengali translation.

## 2. First run (প্রথম চালু)
Start Lumina, then press `Ctrl+O`, drop a file onto the window, or run `lumina movie.mkv`. Opening another file while
Lumina is running sends it to the existing window. The home screen shows Continue watching, Recent and Favorites.
Controls fade out when the mouse is idle; move the mouse to bring them back.

## 3. Features and default shortcuts (সব ফিচার)
All shortcuts can be changed in Settings > Shortcuts (conflicts are detected). Defaults:

| Area | Action | Key |
|---|---|---|
| Playback | Play / pause | Space |
| | Seek back / forward 5 s | Left / Right |
| | Previous / next frame | `,` / `.` |
| | Slower / faster / normal speed | X / C / Z |
| | A-B loop: set A / set B / clear | `[` / `]` / Backspace |
| | Next / previous chapter | PgDown / PgUp |
| | Add bookmark | B |
| Audio | Volume up / down | Up / Down (also mouse wheel over the video) |
| | Mute | M |
| | Next audio track | A |
| Subtitles | Next subtitle track | S |
| | Show / hide subtitles | W / Shift+W |
| | Subtitle delay -0.1 s / +0.1 s | G / H |
| Window | Fullscreen / exit fullscreen | F / Esc |
| | Always on top | T |
| | Playback statistics overlay | I |
| | Screenshot | Ctrl+S |
| Library | Open file / Open URL | Ctrl+O / Ctrl+U |
| | Open library / Settings | L / Ctrl+, |
| Playlist | Toggle playlist | P |
| | Next / previous item | Ctrl+Right / Ctrl+Left |
| | Shuffle / cycle repeat | Shift+S / R |

Other features, reachable from the control bar and menus:
- **Seek bar:** hover for a thumbnail preview; chapter marks show on the bar. Dragging does fast keyframe seeks, releasing seeks exactly.
- **Playlist:** drag to reorder, search, shuffle, repeat, save/load M3U, auto-advance. Playback position is remembered per file.
- **Tracks:** audio and subtitle menus show language, codec and layout. Add external subtitles from the subtitle menu; matching files next to the video are found automatically.
- **Equalizer:** 10 bands. **Video adjust:** brightness, contrast and more, with presets Default / Cinema / Vivid / Night. Deinterlace only helps old interlaced TV/DVD video.
- **Bookmarks and A-B loop** per file. **Mini-player** mode for a small floating window.
- **Volume:** on Linux with PulseAudio/PipeWire the slider, keys and wheel control the system volume; elsewhere they control the player volume (up to 150%).
- **Per-file settings:** volume, speed, tracks and delays can be remembered per file (Settings > Playback).

## 4. Settings pages (সেটিংস)
`Ctrl+,` opens Settings; the search box finds any option; each page has Reset.
General (language, startup, history, recent files) · Playback (hardware decoding, default speed, seek step, per-file settings, auto-next, seek preview) ·
Video · Audio · Subtitles (size, colors, languages) · Library (folders to scan) · Appearance (Dark / Light / OLED, reduced motion) ·
Shortcuts · Advanced (cache size, readahead, verbose log, open logs folder, engine overrides) · About.

Engine overrides (Advanced) take raw mpv options, one `name=value` per line. They are for experts: a wrong option can break playback. Clear the box and restart to undo.

## 5. Library (লাইব্রেরি)
Add folders in Settings > Library; Lumina scans them in the background. Press `L` to browse, search, and mark favorites.
The library is optional: if its database cannot be opened, playback still works.

## 6. Network streams (নেটওয়ার্ক স্ট্রিম)
`Ctrl+U` accepts http(s), rtmp, rtsp, ftp and file URLs. Direct media links and HLS/DASH work out of the box.
Web pages (for example video sites) need `yt-dlp` installed and on your PATH (`brew install yt-dlp`, `sudo dnf install yt-dlp`); Lumina finds it automatically.
Only open URLs you trust.

## 7. Troubleshooting (সমস্যা সমাধান)
- **No sound (শব্দ নেই):** On Fedora the default `ffmpeg-free` cannot decode E-AC3/DTS. Enable RPM Fusion and run
  `sudo dnf install libavcodec-freeworld`. Lumina shows a banner with this command when it happens.
- **High CPU, stats overlay (`I`) shows software decoding:** VAAPI needs the full driver on Fedora:
  `sudo dnf install mesa-va-drivers-freeworld`, then check `vainfo` lists your codec (for example H264 High VLD).
  Press `I` in Lumina and look for `vaapi` in the hwdec line.
- **Logs:** Settings > Advanced > Open logs folder (rotating files, 5 x 1 MB). Typical locations:
  Linux `~/.local/share/mdmakky/Lumina Player/logs` (Flatpak: under `~/.var/app/io.github.mdmakky.Lumina/data/`), macOS `~/Library/Application Support/mdmakky/Lumina Player/logs`. If unsure, use the Open logs folder button.
  Turn on "verbose log" in Advanced for more detail.
- **Collect a bug report on Linux:** `tools/diagnose-linux.sh /real/path/to/movie.mkv` prints system, codec, audio and VAAPI info.
- **Video upside down or wrong:** run with `LUMINA_FLIP_Y=1` and report it.
- **Bengali not available:** build with `qt6-linguist` and `qt6-qttools-devel` installed.
- **Always on top does nothing:** some Wayland compositors (GNOME) ignore it.

## 8. FAQ
**Why not just use mpv/VLC? (কেন Lumina?)** Lumina uses mpv's engine with a simpler interface and special care for seek smoothness; VLC comparison numbers are still being collected (`docs/PERFORMANCE_REPORT.md`).
**Does it collect data? (ডেটা সংগ্রহ?)** No telemetry. Network access happens only when you open a network URL.
**Where is my data? (ডেটা কোথায়?)** Settings in the Qt settings store; history and library in `library.db` in the app data folder. Delete them to reset.
**Which formats? (ফরম্যাট)** Anything mpv/FFmpeg plays. Missing codecs usually mean a limited system FFmpeg (see Troubleshooting).
**Is it free? (ফ্রি?)** Yes, GPL-3.0-or-later.
