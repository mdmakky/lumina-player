#pragma once

#include "PlatformIntegration.h"

#include <QVariantMap>

// Pure MPRIS2 state mapping (no D-Bus types) so it is unit-testable everywhere.
namespace Mpris {

inline const QString kTrackPath = QStringLiteral("/io/github/mdmakky/Lumina/track/current");

QString playbackStatus(const MediaState &s);   // "Playing" | "Paused" | "Stopped"
QVariantMap metadata(const MediaState &s);     // mpris:trackid (path string), mpris:length (us), xesam:title/url
qlonglong toMicros(double seconds);
// Player-interface properties that differ between two states (position excluded).
QVariantMap changedProperties(const MediaState &from, const MediaState &to);
// Whether the screen must be kept awake.
bool wantsInhibit(const MediaState &s);
// Resolves Seek(offset us) into an absolute target in seconds, clamped to [0, duration].
double seekTarget(const MediaState &s, qlonglong offsetMicros);

} // namespace Mpris
