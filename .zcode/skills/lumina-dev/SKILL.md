---
name: lumina-dev
description: Build, test, verify, and develop the Lumina Player media player project (C++20/Qt6/libmpv). Use whenever working on this repository — building, running tests, adding features, or debugging playback.
---

# Lumina Player Development

Read `AGENTS.md` in the repo root FIRST — it has the architecture rules, layer boundaries, and current milestone status.

## Build & verify (always do this before committing)

```bash
# Configure once per worktree
cmake --preset linux-debug

# Build (fast, incremental)
cmake --build --preset linux-debug

# Unit tests
ctest --preset linux-debug
```

Binary: `./build/linux-debug/lumina`

## Playback verification (proves the engine actually works)

Generate test media once (Fedora free ffmpeg has no x264 — use mpeg4):

```bash
mkdir -p /tmp/lumina-test
ffmpeg -y -f lavfi -i testsrc2=size=1280x720:rate=30 \
  -f lavfi -i "sine=frequency=440:duration=15" -t 15 \
  -c:v mpeg4 -q:v 4 -pix_fmt yuv420p -c:a aac /tmp/lumina-test/sample.mp4
```

Run the built-in playback self-test (load → play → seek → pause → resume → verdict):

```bash
LUMINA_PLAYBACK_TEST=1 timeout 20 ./build/linux-debug/lumina /tmp/lumina-test/sample.mp4
echo "exit=$?"   # 0 = PASSED, 1 = FAILED, 124 = hung (bug!)
```

The log shows `t=Ns pos=... dur=... paused=...` state snapshots each second. The verdict line is `Playback self test PASSED|FAILED`.

Debug logging: `LUMINA_DEBUG=1 ./lumina` (enables lumina.* debug categories).

## Environment gotchas (verified on this Fedora 45 machine)

- Do NOT `dnf install ffmpeg-devel` — it conflicts with Fedora's free ffmpeg libs (rpmfusion). mpv-devel already provides the playback stack.
- mpv requires `LC_NUMERIC=C`; the engine calls `setlocale` itself — never remove that.
- Qt scene graph must be OpenGL (`QQuickWindow::setGraphicsApi` in main.cpp) — mpv render API requires it. Do not change.
- Our own message handler forces stderr logging — Qt's default stays silent when stderr isn't a console.
- Screenshot tools (grim) don't work on this compositor; use the self-test harness for verification instead.

## Commit conventions

- Small logical commits: `feat:`, `fix:`, `docs:`, `build:`, `test:`, `refactor:`, `chore:`
- Never commit untested code — build + ctest + (if playback-related) self-test must pass first.
- Update `AGENTS.md` §4 (status) and `docs/ROADMAP.md` when completing a milestone item.
