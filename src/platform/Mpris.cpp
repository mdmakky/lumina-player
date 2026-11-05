#include "Mpris.h"

#include <algorithm>

namespace Mpris {

QString playbackStatus(const MediaState &s)
{
    switch (s.status) {
    case MediaState::Playing: return QStringLiteral("Playing");
    case MediaState::Paused: return QStringLiteral("Paused");
    default: return QStringLiteral("Stopped");
    }
}

qlonglong toMicros(double seconds) { return qlonglong(seconds * 1e6); }

QVariantMap metadata(const MediaState &s)
{
    QVariantMap m;
    if (s.status == MediaState::Stopped && s.url.isEmpty())
        return m;
    m[QStringLiteral("mpris:trackid")] = kTrackPath;
    if (s.duration > 0)
        m[QStringLiteral("mpris:length")] = toMicros(s.duration);
    if (!s.title.isEmpty())
        m[QStringLiteral("xesam:title")] = s.title;
    if (!s.url.isEmpty())
        m[QStringLiteral("xesam:url")] = s.url.toString(QUrl::FullyEncoded);
    return m;
}

QVariantMap changedProperties(const MediaState &a, const MediaState &b)
{
    QVariantMap c;
    if (a.status != b.status)
        c[QStringLiteral("PlaybackStatus")] = playbackStatus(b);
    if (metadata(a) != metadata(b))
        c[QStringLiteral("Metadata")] = metadata(b);
    if (a.volume != b.volume)
        c[QStringLiteral("Volume")] = b.volume;
    if (a.rate != b.rate)
        c[QStringLiteral("Rate")] = b.rate;
    if (a.canNext != b.canNext)
        c[QStringLiteral("CanGoNext")] = b.canNext;
    if (a.canPrevious != b.canPrevious)
        c[QStringLiteral("CanGoPrevious")] = b.canPrevious;
    return c;
}

bool wantsInhibit(const MediaState &s) { return s.status == MediaState::Playing && s.hasVideo; }

double seekTarget(const MediaState &s, qlonglong offsetMicros)
{
    const double t = s.position + offsetMicros / 1e6;
    return std::clamp(t, 0.0, s.duration > 0 ? s.duration : std::max(t, 0.0));
}

} // namespace Mpris
