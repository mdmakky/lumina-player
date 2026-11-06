#pragma once

#include <QSet>
#include <QString>

namespace Lumina {

// Options that must not be settable from the user-editable 'mpv options' box:
// they load scripts/configs, open a control socket, write files or run external
// programs, so a tampered settings file could turn into code execution.
inline bool isBlockedMpvOption(const QString &rawName)
{
    static const QSet<QString> blocked{
        QStringLiteral("script"),           QStringLiteral("scripts"),        QStringLiteral("script-opts"),
        QStringLiteral("load-scripts"),     QStringLiteral("include"),        QStringLiteral("config"),
        QStringLiteral("config-dir"),       QStringLiteral("input-conf"),     QStringLiteral("input-file"),
        QStringLiteral("input-commands"),   QStringLiteral("input-ipc-server"), QStringLiteral("input-ipc-client"),
        QStringLiteral("ytdl-raw-options"), QStringLiteral("ytdl-format-exec"), QStringLiteral("log-file"),
        QStringLiteral("profile"),          QStringLiteral("watch-later-directory"),
    };
    QString name = rawName.trimmed().toLower();
    while (name.startsWith(QLatin1Char('-')))
        name.remove(0, 1);
    name.replace(QLatin1Char('_'), QLatin1Char('-'));
    return blocked.contains(name) || name.startsWith(QLatin1String("script-opts")) || name.startsWith(QLatin1String("input-ipc"));
}

} // namespace Lumina
