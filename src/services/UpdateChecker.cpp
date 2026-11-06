#include "UpdateChecker.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrl>

namespace {
struct Ver { int n[3] = {0, 0, 0}; bool pre = false; };
Ver parse(QString s)
{
    Ver v;
    s = s.trimmed();
    if (s.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) s.remove(0, 1);
    const int dash = s.indexOf(QLatin1Char('-'));
    if (dash >= 0) { v.pre = true; s.truncate(dash); }
    const QStringList parts = s.split(QLatin1Char('.'));
    for (int i = 0; i < 3 && i < parts.size(); ++i) v.n[i] = parts[i].toInt();
    return v;
}
} // namespace

UpdateChecker::UpdateChecker(const QString &currentVersion, QObject *parent)
    : QObject(parent), m_current(currentVersion)
{
}

bool UpdateChecker::isNewer(const QString &candidate, const QString &current)
{
    const Ver a = parse(candidate), b = parse(current);
    for (int i = 0; i < 3; ++i)
        if (a.n[i] != b.n[i]) return a.n[i] > b.n[i];
    return b.pre && !a.pre; // 0.3.1 final is newer than 0.3.1-preview
}

bool UpdateChecker::isTrustedReleaseUrl(const QString &url)
{
    return url.startsWith(QLatin1String("https://github.com/mdmakky/lumina-player/"));
}

bool UpdateChecker::parseLatest(const QByteArray &json, QString *tag, QString *url)
{
    const QJsonArray arr = QJsonDocument::fromJson(json).array();
    for (const QJsonValue &v : arr) {
        const QJsonObject o = v.toObject();
        if (o.value(QLatin1String("draft")).toBool()) continue;
        const QString t = o.value(QLatin1String("tag_name")).toString();
        const QString u = o.value(QLatin1String("html_url")).toString();
        if (t.isEmpty() || !isTrustedReleaseUrl(u)) continue;
        *tag = t;
        *url = u;
        return true;
    }
    return false;
}

void UpdateChecker::check()
{
    if (m_checking) return;
    if (!m_net) m_net = new QNetworkAccessManager(this);
    m_checking = true;
    m_status = tr("Checking…");
    emit changed();

    QNetworkRequest req(QUrl(QStringLiteral("https://api.github.com/repos/mdmakky/lumina-player/releases?per_page=5")));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("LuminaPlayer/") + m_current);
    req.setRawHeader("Accept", "application/vnd.github+json");
    req.setTransferTimeout(10000);
    QNetworkReply *reply = m_net->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        m_checking = false;
        QString tag, url;
        if (reply->error() != QNetworkReply::NoError) {
            m_status = tr("Could not check for updates (%1).").arg(reply->errorString());
        } else if (!parseLatest(reply->readAll(), &tag, &url)) {
            m_status = tr("No published release yet.");
        } else if (isNewer(tag, m_current)) {
            m_available = true;
            m_latest = tag;
            m_url = url;
            m_status = tr("Version %1 is available.").arg(tag);
        } else {
            m_available = false;
            m_status = tr("You are up to date (%1).").arg(m_current);
        }
        emit changed();
    });
}
