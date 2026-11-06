# Security Review (paper review, 2026-11-06)

Scope: Lumina Player 0.3.x at commit `3cd1a25`. Method: read the code paths named below; nothing was fuzzed or run.
Code was not changed; fixes are listed at the end. Lumina has no server component and no telemetry.

## Threat model
Assets: the user's files and session. Attackers: (a) a malicious media or subtitle file or stream the user opens, (b) another local
user or process on the same machine, (c) a malicious web page/URL fed to the URL box. Out of scope: a compromised user account.

## Checklist and findings

| # | Area | Status | Evidence / finding |
|---|---|---|---|
| 1 | Untrusted media files | Accepted risk | All demuxing/decoding is FFmpeg inside libmpv, in-process. A decoder bug is code execution as the user. Mitigation is keeping FFmpeg/mpv updated (distro packages; Flatpak runtime). No sandbox on macOS/native Linux. |
| 2 | Subtitle parsing (libass, mpv) | Accepted risk | In-process via mpv. Embedded fonts in MKV are loaded by libass/FreeType. Same mitigation as 1. Lumina does not parse subtitles itself (`MpvEngine` only passes paths). |
| 3 | Network URLs | Partly OK | `UrlUtils::parseMediaUrl` (used by Open URL) is strict: rejects spaces, requires host, allows only http/https/rtmp(s/e)/rtsp(s)/ftp/file. **But** the command line and single-instance paths do not use it (finding S2). |
| 4 | yt-dlp subprocess | OK with caveat | Lumina never spawns yt-dlp itself: `MpvEngine.cpp` (~l.79-86) finds `yt-dlp`/`youtube-dl` with `QStandardPaths::findExecutable` over `$PATH` first and then `/opt/homebrew/bin`, `/usr/local/bin`, `/usr/bin`, and hands the absolute path to mpv via `script-opts=ytdl_hook-ytdl_path`. mpv runs it with arguments (no shell). Caveat: mpv sends *any* non-direct http(s) URL to yt-dlp, which fetches arbitrary remote extractors; and a user-writable directory early in `$PATH` wins over system locations (finding S5). |
| 5 | Single-instance socket | Mostly OK | `SingleInstance::listen` sets `QLocalServer::UserAccessOption` (owner-only socket). On a stale socket it calls `removeServer` and relistens (symlink/race in shared `/tmp` is mitigated by the 0700 option but the name is a fixed string; use `$XDG_RUNTIME_DIR` when present, finding S6). Server reads the whole message with `readAll()` and no size limit (finding S4). |
| 6 | Single-instance payload | **Weak** | Received strings go to `openCommandLineFiles` -> `QUrl::fromUserInput` -> `openFiles` with no scheme allow-list, so a local process of the same user can make the player open `edl://`, `av://lavfi`, `lavf://`, `ytdl://`, `smb://` etc. mpv protocols (finding S2). Same user only, so low impact, but the Open URL validation is bypassed. |
| 7 | MPRIS exposure | OK | Standard MPRIS2 over the *session* bus (`MprisIntegration.cpp`): any app of the same user can play/pause/seek/OpenUri. `OpenUri` (`MprisIntegration.cpp` l.135) uses `QUrl::fromUserInput` with no scheme check, so any session-bus client can make the player open any mpv protocol (finding S2). `CanQuit`/`Raise` should not expose anything else. The Flatpak only owns its own bus name. |
| 8 | OS volume via `pactl` | OK | `QProcess::start("pactl", fixed arguments)`; the only variable is a number 0-100. Resolved via PATH (low risk; same as any CLI tool). |
| 9 | Settings and DB file permissions | **Weak** | `QSettings` (INI/plist) and `library.db` (`Database.cpp`, AppDataLocation) are created with the process umask; there is no explicit `chmod 0600` / `setPermissions`. Default umask 022 makes them world-readable (watch history, file paths). Directory is under the user home (usually 0700 on macOS, 0755/0700 on Linux depending on distro). Finding S3. `HistoryRepository` uses `prepare` with `?` placeholders for all values (SQL injection not found there; `LibraryRepository` was not re-read). |
| 10 | Settings > Advanced mpv overrides | Accepted, needs warning | `PlayerController.cpp` ~l.153 applies every `name=value` line via `setMpvOption`. Options such as `script`, `scripts`, `include`, `input-ipc-server`, `ytdl-raw-options`, `lavfi-complex` give arbitrary code execution or a control socket. Settings are the user's own, but a tampered settings file (see 9) or a shared-config scenario would turn this into code execution. Mitigation: block an explicit deny-list (`script*`, `include`, `input-ipc-*`, `input-conf`, `ytdl-path`, `load-*`), plus a warning text (finding S1). `LUMINA_MPV_OPTS` env has the same power but needs env control. |
| 11 | Flatpak permissions | Review | `finish-args`: ipc, wayland, fallback-x11, dri, pulseaudio, **network**, read-only videos/music/download, talk-name ScreenSaver + SessionManager. Good: no `--filesystem=home`, no `--device=all`. Gaps: the MPRIS bus name (`--own-name=org.mpris.MediaPlayer2.lumina`) is not declared, so MPRIS will probably not work in the sandbox; arbitrary folders need the file portal (fine). `fallback-x11` with `ipc` weakens isolation only when running on X11. |
| 12 | Logging | OK | Logs rotate (5 x 1 MB); verbose mode logs mpv messages that may contain file paths and URLs (including query tokens). Mention in Settings text (finding S7). |
| 13 | Updates / supply chain | OK | No auto-updater, no remote code loading. QML is compiled into the binary. Build dependencies come from distro/Flathub/Homebrew; Flatpak sources should stay pinned by hash. |
| 14 | Crash handling / drag & drop | OK | Dropped paths go through the same open path as the CLI; there is no `file://` write capability. Screenshots write only into the configured folder. |

## Prioritized fixes (not yet done)
1. **S1 (high):** deny-list dangerous mpv options in Advanced overrides and in `LUMINA_MPV_OPTS`; add a warning line in the UI.
2. **S2 (high):** validate every externally supplied URL (CLI args, single-instance payload, MPRIS `OpenUri` which is `QUrl::fromUserInput` today, drag & drop) against an allow-list of schemes: local file, http(s), rtmp(s), rtsp(s). Reject `edl`, `av`, `lavf`, `ytdl`, `memory`, `fd`, `appending`, `hex`, etc.
3. **S3 (medium):** create the app-data directory 0700 and set `library.db`, the settings file and logs to 0600 at startup (`QFile::setPermissions`).
4. **S4 (medium):** cap the single-instance message (e.g. 64 KB) and the number of arguments; add a read timeout.
5. **S5 (low):** search the fixed system locations (`/usr/bin`, `/usr/local/bin`, Homebrew) before `$PATH`, and skip relative `$PATH` entries; show the resolved path in About.
6. **S6 (low):** put the single-instance socket in `$XDG_RUNTIME_DIR`; keep `UserAccessOption`.
7. **S7 (low):** note in verbose-log help that logs can contain URLs/paths; strip URL query strings in logs.
8. **S8 (Flatpak):** add `--own-name=org.mpris.MediaPlayer2.lumina` (the name registered in `MprisIntegration.cpp` l.151; no `own-name` is in the manifest today) and test MPRIS inside the sandbox.
9. **S9 (process):** run the app under ASan/UBSan once with a corpus of malformed media and subtitle files, and keep distro/runtime FFmpeg current (release notes should tell users to update).
