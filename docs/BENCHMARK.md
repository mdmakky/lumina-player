# Seek benchmark: Lumina vs mpv vs VLC (Fedora)

Goal: prove (or disprove) that Lumina seeks as smoothly as mpv and better than
VLC on your Fedora machine, using identical files and the same seek pattern.

**Verification status.** Everything below was developed and run on macOS only
(VideoToolbox). Nothing in this document has been run on Linux. Unverified on
Linux: the VAAPI/NVDEC hwdec ladder, `vd-lavc-software-fallback`, the VLC
script (`tools/vlc_seekbench.py`), the mpv script on Linux, and every number you
will produce. Treat the first run as the real verification.

Reference run (macOS, VideoToolbox, 4K60, ms; `LUMINA_SEEK_COALESCE=0` vs on):

| file | exact p50/p95 | keyframe p50/p95 | spam settle |
|---|---|---|---|
| h264 GOP250, off | 1043 / 1902 | 67 / 286 | 2082 |
| h264 GOP250, on | 1060 / 1869 | 67 / 271 | 2076 |
| hevc10 GOP2s, off | 142 / 211 | 42 / 241 | 2506 |
| hevc10 GOP2s, on | 153 / 215 | 40 / 44 | 1386 |

mpv's own seek merging already makes spam cheap; the main win is that
timeline dragging now does live keyframe seeks (about 40-70 ms) instead of none.

## 1. Prerequisites (Fedora)

```bash
sudo dnf install ffmpeg-free mpv vlc python3-vlc   # RPM Fusion ffmpeg/vlc are fine too
# VAAPI check: should list profiles (H264/HEVC/VP9/AV1)
vainfo
```

Build Lumina (see `docs/BUILDING.md`), binary: `build/linux-debug/lumina`.

## 2. Generate the test files (same for all players)

Software-encoded, so the content is identical everywhere. 60 s, 4K60:

```bash
mkdir -p ~/seekbench && cd ~/seekbench
SRC="-f lavfi -i testsrc2=size=3840x2160:rate=60 -f lavfi -i sine=f=440 -t 60"
ffmpeg $SRC -c:v libx264 -preset ultrafast -g 120 -pix_fmt yuv420p -c:a aac h264_gop2s.mp4
ffmpeg $SRC -c:v libx264 -preset ultrafast -g 250 -keyint_min 250 -sc_threshold 0 \
       -pix_fmt yuv420p -c:a aac h264_gop250.mp4          # worst case for exact seeks
ffmpeg $SRC -c:v libx265 -preset ultrafast -x265-params keyint=120:log-level=error \
       -pix_fmt yuv420p10le -c:a aac hevc10_gop2s.mp4     # 10-bit HEVC
```

Also run on one real file you complain about (a long movie). The synthetic files
are the reproducible part.

## 3. Lumina

```bash
for f in h264_gop2s h264_gop250 hevc10_gop2s; do
  LUMINA_PLAYBACK_TEST=1 LUMINA_SEEK_BENCH=1 \
  LUMINA_SEEK_BENCH_CSV=lumina_$f.csv \
  /path/to/build/linux-debug/lumina ~/seekbench/$f.mp4 2>&1 | grep -E "SEEKBENCH|self test"
done
```

Options: `LUMINA_SEEK_BENCH_N=40` (seeks per mode). `LUMINA_SEEK_COALESCE=0`
turns the seek coalescing off, to see the "before" behaviour. Run alone: do not
touch the mouse/keyboard, do not run other players meanwhile.

### Reading the output

One line, example shape:

```
SEEKBENCH n=40 hwdec=vaapi codec=h264 exact[p50=.. p95=.. max=.. drops=..] keyframe[p50=.. p95=.. max=.. drops=..] spam[settle_ms=.. restarts=.. drops=..] timeouts=0 (latencies in ms)
```

- `exact`: absolute+exact seek (what you get when you release the timeline or
  click). Latency = seek command until mpv's playback-restart, i.e. the new
  picture is ready to show. **p95 and max are what you feel as a "freeze".**
- `keyframe`: absolute+keyframes (used while dragging the timeline). Should be
  much lower than exact on long-GOP files.
- `spam`: 120 exact seeks sent 10 ms apart (a held arrow key / wild drag).
  `settle_ms` = time until the last restart; `restarts` = how many seeks mpv
  actually executed (with coalescing this is far below 120).
- `drops`: dropped frames (decoder + display) during the mode.
- `hwdec`: decoder actually used. It must say `vaapi` (or `nvdec`) on Fedora;
  `no`/empty means software decode, which is the first thing to investigate.
- `timeouts`: seeks that got no restart within 5 s. Must be 0.
- Verdict line `Playback self test PASSED` requires no error, no timeouts and
  p95 < 3000 ms for both modes (deliberately generous; it is a smoke gate).
- The CSV has one row per seek: `mode,index,target_s,latency_ms`.

## 4. mpv (reference)

```bash
for f in h264_gop2s h264_gop250 hevc10_gop2s; do
  mpv --no-config --hwdec=vaapi --script=tools/mpv_seekbench.lua \
      --script-opts=seekbench-csv=mpv_$f.csv ~/seekbench/$f.mp4 | grep SEEKBENCH
done
```

`--no-config` gives mpv stock defaults; the script passes explicit `exact` /
`keyframes` flags so the comparison with Lumina is like for like. It has no
spam mode and no drop counter.

## 5. VLC

```bash
python3 tools/vlc_seekbench.py ~/seekbench/h264_gop250.mp4 40 vlc_h264_gop250.csv
```

VLC has no playback-restart event, so latency is "set_time until get_time is
within 1 s of the target while Playing": this is a slight under-estimate of the
true time-to-picture. For the subjective test also do this by hand in each
player, on `h264_gop250.mp4`: drag the timeline back and forth quickly for 10 s,
then hold Right-arrow for 10 s. Note whether the picture freezes, stutters or
audio drops out. Optionally capture `vlc -vvv` output for decoder errors.

## 6. What to report back

For each file and player, paste:

1. The `SEEKBENCH` line (Lumina and mpv) / the VLC line.
2. The CSVs (or just p50/p95/max if you prefer).
3. `hwdec=` as reported by Lumina, and `vainfo | head -20`.
4. Any `timeouts`, `FAILED`, or a hwdec that fell back to software.
5. Subjective notes from the manual drag/hold-key test, per player.
6. GPU/driver (`lspci | grep -i vga`, `rpm -q mesa-va-drivers`), Wayland or X11.

Interpretation: Lumina should be within about 20% of mpv on `exact` p95, and
its `spam` settle time should be dominated by one or two seeks, not by 120. If
Lumina is much slower than mpv on the same file, it is probably not on VAAPI
(look at `hwdec=`) or the VAAPI driver rejects the stream (try
`LUMINA_MPV_OPTS=hwdec=no` to compare against software).
