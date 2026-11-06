#pragma once

#include <QString>

// Minimal, local-only crash reporting: on a fatal signal (or unhandled exception on Windows) a short
// report (version, time, signal, native backtrace where available) is written to
// <logs>/crash-last.txt. Nothing is sent anywhere. On the next start takePreviousReport() moves it to
// crash-<timestamp>.txt and returns that path so the UI can tell the user where it is.
namespace CrashReporter {

void install(const QString &logsDir, const QString &version);
// Empty when the previous run ended normally.
QString takePreviousReport(const QString &logsDir);

} // namespace CrashReporter
