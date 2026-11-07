#pragma once

#include <QObject>
#include <QString>

class QProcess;

// Fedora only: enables RPM Fusion (free) and installs libavcodec-freeworld so E-AC3/DTS/H.264 play.
// Runs one fixed command through pkexec (the system's own polkit password prompt). Nothing else can be
// passed in: the only variable is the Fedora release number, validated as digits. Not used in Flatpak.
class CodecFixer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY changed FINAL)
    Q_PROPERTY(QString message READ message NOTIFY changed FINAL)

public:
    explicit CodecFixer(QObject *parent = nullptr) : QObject(parent) {}

    bool running() const { return m_running; }
    QString message() const { return m_message; }

    // True on a Fedora host (not inside a Flatpak sandbox) where the helper can work.
    Q_INVOKABLE bool available() const;
    Q_INVOKABLE void start();

    // The shell script run as root; empty if `fedoraRelease` is not 2-3 digits.
    static QString buildScript(const QString &fedoraRelease);

signals:
    void changed();
    void finished(bool ok);

private:
    QProcess *m_proc = nullptr;
    bool m_running = false;
    QString m_message;
};
