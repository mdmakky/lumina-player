#!/usr/bin/env python3
"""VLC counterpart of Lumina's seek benchmark (needs `pip install python-vlc`).

Latency = set_time() -> first time get_time() reports a position within 1 s of
the target while the player is Playing. Approximates "picture is at the new
position"; VLC exposes no playback-restart event. UNVERIFIED on Linux.

Usage: tools/vlc_seekbench.py FILE [N=40] [CSV]
"""
import math, random, sys, time
import vlc

path = sys.argv[1]
n = int(sys.argv[2]) if len(sys.argv) > 2 else 40
csv = sys.argv[3] if len(sys.argv) > 3 else None

player = vlc.MediaPlayer(path)
player.play()
while player.get_length() <= 0:
    time.sleep(0.1)
dur = player.get_length() / 1000.0
time.sleep(1.0)


def pct(v, p):
    v = sorted(v)
    return v[max(0, math.ceil(p * len(v)) - 1)] if v else 0.0


rows, lat = [], []
for k in range(n):
    target = 1 + random.random() * (dur - 6)
    t0 = time.monotonic()
    player.set_time(int(target * 1000))
    ms = 5000.0
    while time.monotonic() - t0 < 5:
        if player.get_state() == vlc.State.Playing and abs(player.get_time() / 1000.0 - target) < 1.0:
            ms = (time.monotonic() - t0) * 1000
            break
        time.sleep(0.002)
    lat.append(ms)
    rows.append(f"vlc,{k},{target:.3f},{ms:.1f}")
    time.sleep(0.15)

print(f"SEEKBENCH n={n} player=vlc p50={pct(lat, .5):.1f} p95={pct(lat, .95):.1f} max={max(lat):.1f} (ms)")
if csv:
    with open(csv, "w") as f:
        f.write("mode,index,target_s,latency_ms\n" + "\n".join(rows) + "\n")
player.stop()
