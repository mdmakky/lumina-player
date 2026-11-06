# Changelog

## 0.4.0 (unreleased)

Features and fixes since 0.3.0:
- Network streams: Open URL dialog (Ctrl+U), yt-dlp for page URLs, buffering, per-file playback profiles
- Seek-bar hover thumbnail preview (second lightweight mpv handle, LRU cache)
- Linux integration: MPRIS2, screensaver inhibit, system volume sync via pactl (slider, keys, wheel, laptop keys); new volume bar and volume OSD
- Rotating file logs (5 x 1 MB) and "Open logs folder"; mpv warnings forwarded to the log
- Bengali translation with live language switching (Qt Linguist pipeline)
- Track labels with language names, codec and layout; unified popup panels; video adjustment presets (Default/Cinema/Vivid/Night)
- "No sound" diagnostic banner with distro-specific fix (libavcodec-freeworld); `tools/diagnose-linux.sh`, `tools/install-deps-fedora.sh`
- Flatpak manifest now builds and runs on Fedora (KDE 6.10 runtime, libplacebo, Wayland + X11 verified, VAAPI works)
- Windows build files and experimental CI job; macOS GL 3.3 core profile and coreaudio fallback
- Fixes: menus are modal (outside click no longer pauses), chrome auto-hide re-arms, smaller default subtitle size, upside-down video, Qt 6.4 compatibility
- Release preparation: user guide, security review, license audit + SBOM generator (`tools/make-sbom.sh`), QA and performance reports, RPM/DEB/AppImage recipes (unbuilt)

## 0.3.0 (unreleased)

- New floating-overlay player UI, home screen with Continue watching / Recent / Favorites
- Playlist: drag-reorder, shuffle, repeat, search, M3U save/load, auto-advance, resume position
- Chapters (timeline markers, menu, PgUp/PgDn), A-B loop, bookmarks, screenshots
- Audio/subtitle track menu, external subtitles, delay controls, subtitle style, 10-band equalizer
- Settings dialog (10 pages, search, per-page reset), rebindable shortcuts, Dark/Light/OLED themes, reduced motion
- Mini-player mode, single-instance, optional media library
- Performance: seek/cache tuning, seek coalescing + keyframe scrubbing, stats overlay, Linux hwdec ladder
- macOS: coreaudio output (fixes ~4 s speed-change lag), bundle + dmg script
- Packaging: desktop file, AppStream metainfo, Flatpak manifest (unbuilt), CI for Linux (Qt 6.4) and macOS
