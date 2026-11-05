#include "Logger.h"

#include <QLoggingCategory>

#include <QDir>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QStandardPaths>

#include <cstdio>

Q_LOGGING_CATEGORY(lcCore, "lumina.core")
Q_LOGGING_CATEGORY(lcEngine, "lumina.engine")
Q_LOGGING_CATEGORY(lcQml, "lumina.qml")

namespace {

// Rotating file sink: lumina.log, lumina.1.log ... lumina.4.log (5 files x 1 MB).
constexpr int kMaxFiles = 5;
constexpr qint64 kMaxBytes = 1024 * 1024;
QMutex g_fileMutex;
QFile g_file;
QString g_dir;

QString logName(int i) { return g_dir + (i == 0 ? QStringLiteral("/lumina.log") : QStringLiteral("/lumina.%1.log").arg(i)); }

void rotateLocked()
{
    g_file.close();
    QFile::remove(logName(kMaxFiles - 1));
    for (int i = kMaxFiles - 2; i >= 0; --i)
        QFile::rename(logName(i), logName(i + 1));
    g_file.setFileName(logName(0));
    (void)g_file.open(QIODevice::WriteOnly | QIODevice::Append);
}

void writeFile(const QString &line)
{
    QMutexLocker lock(&g_fileMutex);
    if (g_dir.isEmpty())
        return;
    if (!g_file.isOpen() && !g_file.open(QIODevice::WriteOnly | QIODevice::Append))
        return;
    const QByteArray bytes = line.toUtf8() + '\n';
    if (g_file.size() + bytes.size() > kMaxBytes)
        rotateLocked();
    g_file.write(bytes);
    g_file.flush();
}

// Own the output: Qt's default handler stays silent when stderr is not
// detected as a console (CI, service supervisors). We always want logs.
void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    const QString formatted = qFormatLogMessage(type, context, message);
    std::fprintf(stderr, "%s\n", qPrintable(formatted));
    std::fflush(stderr);
    writeFile(formatted);
}

} // namespace

namespace Logger {

void setVerbose(bool on)
{
    QLoggingCategory::setFilterRules(on || qEnvironmentVariableIsSet("LUMINA_DEBUG")
        ? QStringLiteral("lumina.*=true")
        : QStringLiteral("lumina.*.info=true\nlumina.*.debug=false"));
}

QString logsPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/logs");
}

void install()
{
    // Per-app data dir, so UI-test runs (own application name) stay isolated.
    {
        QMutexLocker lock(&g_fileMutex);
        g_dir = logsPath();
        if (!QDir().mkpath(g_dir))
            g_dir.clear();
        else
            g_file.setFileName(logName(0));
    }

    // Qt 6 disables info/debug for custom categories by default — turn info on
    // for ours, keep debug opt-in (LUMINA_DEBUG=1).
    setVerbose(false);

    qSetMessagePattern(
        "[%{time hh:mm:ss.zzz}] %{if-debug}D%{endif}%{if-info}I%{endif}"
        "%{if-warning}W%{endif}%{if-critical}C%{endif}"
        " %{category}: %{message}");

    qInstallMessageHandler(&messageHandler);
}

} // namespace Logger
