#pragma once

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(lcCore)
Q_DECLARE_LOGGING_CATEGORY(lcEngine)
Q_DECLARE_LOGGING_CATEGORY(lcQml)

namespace Logger {

// Installs the application-wide message pattern. Call once at startup.
void install();

} // namespace Logger
