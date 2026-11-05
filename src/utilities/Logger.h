#pragma once

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(lcCore)
Q_DECLARE_LOGGING_CATEGORY(lcEngine)
Q_DECLARE_LOGGING_CATEGORY(lcQml)

namespace Logger {

// Installs the application-wide message pattern. Call once at startup.
void install();
// Folder holding the rotating log files (AppDataLocation/logs).
QString logsPath();
// Debug-level logging for the lumina.* categories (LUMINA_DEBUG=1 keeps it on regardless).
void setVerbose(bool on);

} // namespace Logger
