#include "Logger.h"

#include <QLoggingCategory>

#include <cstdio>

Q_LOGGING_CATEGORY(lcCore, "lumina.core")
Q_LOGGING_CATEGORY(lcEngine, "lumina.engine")
Q_LOGGING_CATEGORY(lcQml, "lumina.qml")

namespace {

// Own the output: Qt's default handler stays silent when stderr is not
// detected as a console (CI, service supervisors). We always want logs.
void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    const QString formatted = qFormatLogMessage(type, context, message);
    std::fprintf(stderr, "%s\n", qPrintable(formatted));
    std::fflush(stderr);
}

} // namespace

namespace Logger {

void install()
{
    // Qt 6 disables info/debug for custom categories by default — turn info on
    // for ours, keep debug opt-in (LUMINA_DEBUG=1).
    const QString rules = qEnvironmentVariableIsSet("LUMINA_DEBUG")
        ? QStringLiteral("lumina.*=true")
        : QStringLiteral("lumina.*.info=true\nlumina.*.debug=false");
    QLoggingCategory::setFilterRules(rules);

    qSetMessagePattern(
        "[%{time hh:mm:ss.zzz}] %{if-debug}D%{endif}%{if-info}I%{endif}"
        "%{if-warning}W%{endif}%{if-critical}C%{endif}"
        " %{category}: %{message}");

    qInstallMessageHandler(&messageHandler);
}

} // namespace Logger
