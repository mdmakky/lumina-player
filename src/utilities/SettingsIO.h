#pragma once

#include <QByteArray>
#include <QString>

class QSettings;

namespace Lumina::SettingsIO {

// Serialises every QSettings key (shortcuts, subtitle style, theme, language...) into one JSON
// document. Watch history, library data, recents, resume positions and window geometry are
// deliberately excluded (see isExcluded).
QByteArray exportJson(const QSettings &s);

// Validates `json` and writes its keys into `s`. Returns the number of keys applied, or -1 with
// `error` set when the document is not a Lumina settings export (nothing is written then).
int importJson(QSettings &s, const QByteArray &json, QString *error = nullptr);

// True for keys that must never travel between machines/profiles.
bool isExcluded(const QString &key);

} // namespace Lumina::SettingsIO
