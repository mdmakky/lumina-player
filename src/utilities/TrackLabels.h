#pragma once

#include <QString>
#include <QVariantList>
#include <QVariantMap>

namespace Lumina::TrackLabels {

// Full English language name for an ISO 639-1/2/3 code ("ben" -> "Bangla"); "" when unknown.
// "und"/"unknown"/empty return "" too (callers decide how to label those).
QString languageName(const QString &code);

// "Mono", "Stereo", "5.1", "7.1" for known layouts, else "N ch"; "" when count is 0.
QString channelLayout(int channelCount);

// Short display name for an mpv codec id ("eac3" -> "E-AC3").
QString codecName(const QString &codec);

// Adds "label" (primary line) and "detail" (secondary line, e.g. "AC3 · 5.1 · Default")
// to every mpv track map. Label+detail pairs are unique within the list ("#2" suffix).
QVariantList describeTracks(const QVariantList &tracks, bool audio);

} // namespace Lumina::TrackLabels
