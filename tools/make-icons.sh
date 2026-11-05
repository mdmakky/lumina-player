#!/bin/sh
# Regenerates PNG sizes (+ macOS .icns) from resources/icons/app/lumina.svg.
# Uses rsvg-convert when present (Linux/CI), else AppKit via swift (macOS).
set -e
cd "$(dirname "$0")/.."
SVG=resources/icons/app/lumina.svg
OUT=resources/icons/app
SIZES="16 32 48 64 128 256 512 1024"
for s in $SIZES; do
  if command -v rsvg-convert >/dev/null; then
    rsvg-convert -w $s -h $s "$SVG" -o "$OUT/lumina-$s.png"
  else
    swift - "$SVG" "$OUT/lumina-$s.png" $s <<'SWIFT'
import AppKit
let a = CommandLine.arguments
let img = NSImage(contentsOfFile: a[1])!
let s = Int(a[3])!
let rep = NSBitmapImageRep(bitmapDataPlanes: nil, pixelsWide: s, pixelsHigh: s, bitsPerSample: 8,
  samplesPerPixel: 4, hasAlpha: true, isPlanar: false, colorSpaceName: .deviceRGB, bytesPerRow: 0, bitsPerPixel: 0)!
NSGraphicsContext.saveGraphicsState()
NSGraphicsContext.current = NSGraphicsContext(bitmapImageRep: rep)
img.draw(in: NSRect(x: 0, y: 0, width: s, height: s))
NSGraphicsContext.restoreGraphicsState()
try! rep.representation(using: .png, properties: [:])!.write(to: URL(fileURLWithPath: a[2]))
SWIFT
  fi
done
if command -v iconutil >/dev/null; then
  SET=$(mktemp -d)/lumina.iconset; mkdir -p "$SET"
  for s in 16 32 128 256 512; do
    cp "$OUT/lumina-$s.png" "$SET/icon_${s}x${s}.png"
  done
  cp "$OUT/lumina-32.png"   "$SET/icon_16x16@2x.png"
  cp "$OUT/lumina-64.png"   "$SET/icon_32x32@2x.png"
  cp "$OUT/lumina-256.png"  "$SET/icon_128x128@2x.png"
  cp "$OUT/lumina-512.png"  "$SET/icon_256x256@2x.png"
  cp "$OUT/lumina-1024.png" "$SET/icon_512x512@2x.png"
  iconutil -c icns "$SET" -o "$OUT/lumina.icns"
fi
