#include "CodecFixer.h"

#include <QFile>
#include <QProcess>
#include <QRegularExpression>
#include <QSysInfo>

QString CodecFixer::buildScript(const QString &fedoraRelease)
{
    static const QRegularExpression digits(QStringLiteral("\\A\\d{2,3}\\z"));
    if (!digits.match(fedoraRelease).hasMatch())
        return {};
    return QStringLiteral(
               "set -e; "
               "rpm -q rpmfusion-free-release >/dev/null 2>&1 || "
               "dnf install -y https://mirrors.rpmfusion.org/free/fedora/rpmfusion-free-release-%1.noarch.rpm; "
               "dnf install -y libavcodec-freeworld")
        .arg(fedoraRelease);
}

bool CodecFixer::available() const
{
#ifdef Q_OS_LINUX
    return QSysInfo::productType() == QLatin1String("fedora")
        && !QFile::exists(QStringLiteral("/.flatpak-info"))
        && !buildScript(QSysInfo::productVersion()).isEmpty();
#else
    return false;
#endif
}

void CodecFixer::start()
{
    if (m_running || !available())
        return;
    const QString script = buildScript(QSysInfo::productVersion());
    m_running = true;
    m_message = tr("Waiting for the administrator password, then installing codecs. This can take a minute.");
    emit changed();

    m_proc = new QProcess(this);
    connect(m_proc, &QProcess::finished, this, [this](int code, QProcess::ExitStatus) {
        const QString err = QString::fromLocal8Bit(m_proc->readAllStandardError()).trimmed();
        const bool ok = code == 0;
        if (ok)
            m_message = tr("Codecs installed. Restart Lumina and play the file again.");
        else if (code == 126 || code == 127)
            m_message = tr("Cancelled.");
        else
            m_message = tr("Install failed (%1). Run tools/install-fedora-runtime.sh in a terminal for details. %2")
                            .arg(code).arg(err.section(QLatin1Char('\n'), -1));
        m_running = false;
        m_proc->deleteLater();
        m_proc = nullptr;
        emit changed();
        emit finished(ok);
    });
    connect(m_proc, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
        if (e != QProcess::FailedToStart)
            return;
        m_message = tr("pkexec is not available. Run tools/install-fedora-runtime.sh in a terminal.");
        m_running = false;
        m_proc->deleteLater();
        m_proc = nullptr;
        emit changed();
        emit finished(false);
    });
    m_proc->start(QStringLiteral("pkexec"), {QStringLiteral("/bin/sh"), QStringLiteral("-c"), script});
}
