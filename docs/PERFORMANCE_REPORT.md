# Performance Report (draft, 2026-11-06)

Only numbers that were actually measured are listed; TODO rows are for the owner. Method and commands: `docs/BENCHMARK.md`.
Latency = seek command until mpv's playback-restart (new picture ready). Lower is better.

## Measured: Lumina

### Fedora 45, AMD APU (VAAPI), Wayland, 1080p H.264 mkv (Fight Club)
| Metric | Result |
|---|---|
| Hardware decode | `vaapi-copy` active (also confirmed inside the Flatpak) |
| Exact seek p50 / p95 | ~265 ms / ~430 ms |
| Keyframe seek p50 | ~35 ms |
| Dropped frames during the run | 0 |
| Seek spam (120 seeks, 10 ms apart) settle time | ~1.7 s |

### macOS, VideoToolbox, synthetic 4K60 (`LUMINA_SEEK_COALESCE=0` vs on)
| File | Exact p50 / p95 (ms) | Keyframe p50 / p95 (ms) | Spam settle (ms) |
|---|---|---|---|
| h264 GOP250, coalescing off | 1043 / 1902 | 67 / 286 | 2082 |
| h264 GOP250, coalescing on | 1060 / 1869 | 67 / 271 | 2076 |
| hevc10 GOP2s, off | 142 / 211 | 42 / 241 | 2506 |
| hevc10 GOP2s, on | 153 / 215 | 40 / 44 | 1386 |

Reading: GOP-250 4K exact seeks are slow on every player (decoder must run from the last keyframe); dragging the timeline
uses keyframe seeks (40-70 ms), which is where the user-visible smoothness comes from. mpv's own seek merging already makes spam cheap.

## Comparison with mpv and VLC (owner to fill)
Same file, same machine, no other apps running. Commands: Lumina `LUMINA_SEEK_BENCH=1`, mpv `tools/mpv_seekbench.lua`, VLC `tools/vlc_seekbench.py`
(needs `sudo dnf install vlc python3-vlc`). VLC latency is slightly under-estimated (no restart event).

| File | Player | Exact p50 / p95 (ms) | Keyframe p50 (ms) | Drops | Notes |
|---|---|---|---|---|---|
| 1080p H.264 movie | Lumina | 265 / 430 | 35 | 0 | measured |
| 1080p H.264 movie | mpv | TODO | TODO | TODO | |
| 1080p H.264 movie | VLC | TODO | TODO | TODO | subjective freeze on drag? |
| h264_gop250 4K60 (synthetic) | Lumina (Linux) | TODO | TODO | TODO | |
| h264_gop250 4K60 (synthetic) | mpv | TODO | TODO | TODO | |
| h264_gop250 4K60 (synthetic) | VLC | TODO | TODO | TODO | |
| hevc10_gop2s 4K60 | Lumina / mpv / VLC | TODO | TODO | TODO | |

Target from the benchmark guide: Lumina within about 20% of mpv on exact p95.

## Other metrics (owner to fill)
| Metric | Lumina | mpv | VLC |
|---|---|---|---|
| Cold start to window (s) | TODO | TODO | TODO |
| Time to first frame, 1080p file (s) | TODO | TODO | TODO |
| CPU % during 1080p H.264 playback (VAAPI) | TODO | TODO | TODO |
| Resident memory after 10 min / after 2 h (MB) | TODO | TODO | TODO |
| Playback statistics overlay (`I`) confirms hwdec on Intel / NVIDIA | TODO | n/a | n/a |

Rules (spec §19): record hardware and file for each row; do not publish unmeasured numbers.
