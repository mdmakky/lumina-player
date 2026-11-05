#include "TimeUtils.h"

#include <QtGlobal>
#include <QtMath>

namespace Lumina {

QString TimeUtils::formatDuration(double seconds)
{
    if (seconds < 0 || !qIsFinite(seconds))
        return QStringLiteral("--:--");

    const qint64 total = qRound64(seconds);
    const int hours = static_cast<int>(total / 3600);
    const int minutes = static_cast<int>((total % 3600) / 60);
    const int secs = static_cast<int>(total % 60);

    if (hours > 0)
        return QStringLiteral("%1:%2:%3")
            .arg(hours)
            .arg(minutes, 2, 10, QChar(u'0'))
            .arg(secs, 2, 10, QChar(u'0'));

    return QStringLiteral("%1:%2").arg(minutes).arg(secs, 2, 10, QChar(u'0'));
}

} // namespace Lumina
