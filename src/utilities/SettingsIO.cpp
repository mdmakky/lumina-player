#include "SettingsIO.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QStringList>

namespace Lumina::SettingsIO {

namespace {
constexpr int kFormatVersion = 1;
const QLatin1String kAppTag("lumina-player-settings");

// Integer-valued JSON numbers come back as double; QSettings consumers expect ints.
QVariant fromJson(const QJsonValue &v)
{
    if (v.isDouble()) {
        const double d = v.toDouble();
        if (d == double(qint64(d)))
            return qint64(d);
    }
    return v.toVariant();
}
}

bool isExcluded(const QString &key)
{
    static const QStringList prefixes{"resume/", "history/", "library/", "window/", "recentFiles", "recentUrls"};
    for (const QString &p : prefixes)
        if (key.startsWith(p))
            return true;
    return false;
}

QByteArray exportJson(const QSettings &s)
{
    QJsonObject values;
    const QStringList keys = s.allKeys();
    for (const QString &k : keys)
        if (!isExcluded(k))
            values.insert(k, QJsonValue::fromVariant(s.value(k)));
    QJsonObject root;
    root.insert(QStringLiteral("app"), kAppTag);
    root.insert(QStringLiteral("version"), kFormatVersion);
    root.insert(QStringLiteral("settings"), values);
    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

int importJson(QSettings &s, const QByteArray &json, QString *error)
{
    auto fail = [&](const QString &msg) {
        if (error) *error = msg;
        return -1;
    };
    QJsonParseError pe;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject())
        return fail(QCoreApplication::translate("SettingsIO", "The file is not valid JSON."));
    const QJsonObject root = doc.object();
    if (root.value(QStringLiteral("app")).toString() != kAppTag || !root.value(QStringLiteral("settings")).isObject())
        return fail(QCoreApplication::translate("SettingsIO", "This is not a Lumina settings file."));
    if (root.value(QStringLiteral("version")).toInt() > kFormatVersion)
        return fail(QCoreApplication::translate("SettingsIO", "The file was made by a newer version of Lumina."));

    const QJsonObject values = root.value(QStringLiteral("settings")).toObject();
    int n = 0;
    for (auto it = values.begin(); it != values.end(); ++it) {
        const QString key = it.key();
        if (key.isEmpty() || key.startsWith(QLatin1Char('/')) || key.contains(QLatin1String("..")) || isExcluded(key)
            || it.value().isNull() || it.value().isObject())
            continue;
        s.setValue(key, fromJson(it.value()));
        ++n;
    }
    return n;
}

} // namespace Lumina::SettingsIO
