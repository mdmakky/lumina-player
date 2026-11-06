#pragma once

#include <QObject>
#include <QString>

class QNetworkAccessManager;

// "Is there a newer release?" via the public GitHub releases API. No telemetry, nothing is
// downloaded or installed: the user is pointed at the release page. Off at start unless the user
// enabled "updates/auto"; the manual check always works.
class UpdateChecker : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool checking READ checking NOTIFY changed FINAL)
    Q_PROPERTY(QString status READ status NOTIFY changed FINAL)
    Q_PROPERTY(bool updateAvailable READ updateAvailable NOTIFY changed FINAL)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY changed FINAL)
    Q_PROPERTY(QString releaseUrl READ releaseUrl NOTIFY changed FINAL)

public:
    explicit UpdateChecker(const QString &currentVersion, QObject *parent = nullptr);

    bool checking() const { return m_checking; }
    QString status() const { return m_status; }
    bool updateAvailable() const { return m_available; }
    QString latestVersion() const { return m_latest; }
    QString releaseUrl() const { return m_url; }

    Q_INVOKABLE void check();

    // "0.3.1-preview" vs "v0.4.0": numeric dotted compare, a pre-release suffix sorts before the final.
    static bool isNewer(const QString &candidate, const QString &current);
    // Only our own repository's release pages are ever opened.
    static bool isTrustedReleaseUrl(const QString &url);
    // Picks the newest non-draft release from the API JSON; false if none.
    // Pre-releases are skipped unless includePrerelease (set when the running build is itself a pre-release).
    static bool parseLatest(const QByteArray &json, QString *tag, QString *url, bool includePrerelease = true);

signals:
    void changed();

private:
    QNetworkAccessManager *m_net = nullptr;
    QString m_current, m_status, m_latest, m_url;
    bool m_checking = false;
    bool m_available = false;
};
