# Testing Strategy

## Layers

| Layer | Tool | Location | Scope |
|---|---|---|---|
| Unit | Qt Test + CTest | `tests/unit/` | Pure logic: time formatting, playlist model, settings parsing |
| Integration | Qt Test | `tests/integration/` (planned) | Database repositories, service classes |
| Playback | Manual + scripted (planned) | `tests/playback/` | mpv-backed scenarios |
| Performance | Benchmark suite (planned) | `tests/performance/` | Startup, time-to-first-frame, seek latency, dropped frames |

## Reference media generation

Test media is generated locally (never committed):

```bash
mkdir -p tests/media && cd tests/media
ffmpeg -f lavfi -i testsrc2=size=1280x720:rate=30 -t 30 -c:v libx264 -pix_fmt yuv420p sample_h264_720p.mp4
ffmpeg -f lavfi -i testsrc2=size=3840x2160:rate=30 -t 10 -c:v libx264 -crf 20 -pix_fmt yuv420p sample_h264_4k.mp4
ffmpeg -f lavfi -i "sine=frequency=440:duration=30" -c:a aac sample_audio.m4a
```

> On Fedora 45, use the mpv-bundled ffmpeg (`ffmpeg` from rpmfusion) — the encoder
> side is only needed for generating local test media, never at build time.

## Coverage targets per milestone

- **M2 (playback):** load → play → pause → seek → resume → EOF → error path (corrupt file).
- **M3 (UX):** shortcut table, idle-hide timing, fullscreen transitions.
- **M5 (performance):** rapid repeated seeking (20 seeks in 10 s), repeated play/pause cycles, seek-completion latency distribution.
- **M7 (library):** scan dedup, resume positions survive restart, corrupt-DB recovery.

## Rules (spec §19)

- Never claim a feature works without testing it.
- Do not fabricate performance results; record hardware and file in every benchmark.
