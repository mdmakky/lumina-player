#include "CrashReporter.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <cstdio>
#include <cstring>
#include <ctime>

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <csignal>
#include <fcntl.h>
#include <unistd.h>
#if __has_include(<execinfo.h>)
#include <execinfo.h>
#define LUMINA_HAVE_BACKTRACE 1
#endif
#endif

namespace {

char g_path[1024];
char g_version[64];

#ifdef Q_OS_WIN
LONG WINAPI onException(EXCEPTION_POINTERS *info)
{
    if (FILE *f = std::fopen(g_path, "w")) {
        std::fprintf(f, "Lumina Player %s\ntime: %lld\nexception: 0x%08lx\n", g_version,
                     static_cast<long long>(std::time(nullptr)),
                     static_cast<unsigned long>(info ? info->ExceptionRecord->ExceptionCode : 0));
        std::fclose(f);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
#else
void writeStr(int fd, const char *s) { if (::write(fd, s, std::strlen(s)) < 0) { /* nothing useful to do */ } }

void onSignal(int sig)
{
    // Async-signal-safe calls only (open/write/backtrace_symbols_fd/signal/raise).
    const int fd = ::open(g_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd >= 0) {
        char num[24];
        int n = 0;
        long t = static_cast<long>(std::time(nullptr));
        char rev[24];
        do { rev[n++] = static_cast<char>('0' + t % 10); t /= 10; } while (t > 0 && n < 22);
        for (int i = 0; i < n; ++i) num[i] = rev[n - 1 - i];
        num[n] = 0;
        writeStr(fd, "Lumina Player ");
        writeStr(fd, g_version);
        writeStr(fd, "\ntime: ");
        writeStr(fd, num);
        writeStr(fd, "\nsignal: ");
        writeStr(fd, sig == SIGSEGV ? "SIGSEGV" : sig == SIGABRT ? "SIGABRT" : sig == SIGBUS ? "SIGBUS"
                                      : sig == SIGFPE ? "SIGFPE" : "SIGILL");
        writeStr(fd, "\nbacktrace:\n");
#ifdef LUMINA_HAVE_BACKTRACE
        void *frames[48];
        const int count = ::backtrace(frames, 48);
        ::backtrace_symbols_fd(frames, count, fd);
#endif
        ::close(fd);
    }
    ::signal(sig, SIG_DFL);
    ::raise(sig);
}
#endif

} // namespace

namespace CrashReporter {

void install(const QString &logsDir, const QString &version)
{
    QDir().mkpath(logsDir);
    const QByteArray path = QDir(logsDir).filePath(QStringLiteral("crash-last.txt")).toLocal8Bit();
    std::snprintf(g_path, sizeof g_path, "%s", path.constData());
    std::snprintf(g_version, sizeof g_version, "%s", version.toLatin1().constData());
#ifdef Q_OS_WIN
    SetUnhandledExceptionFilter(onException);
#else
    struct sigaction sa;
    std::memset(&sa, 0, sizeof sa);
    sa.sa_handler = onSignal;
    sigemptyset(&sa.sa_mask);
#ifdef LUMINA_HAVE_BACKTRACE
    void *warm[1];
    ::backtrace(warm, 1); // first call loads libgcc/allocates: do it now, not inside a crash handler (could deadlock on a heap crash)
#endif
    for (int s : {SIGSEGV, SIGABRT, SIGBUS, SIGFPE, SIGILL})
        sigaction(s, &sa, nullptr);
#endif
}

QString takePreviousReport(const QString &logsDir)
{
    const QFileInfo fi(QDir(logsDir).filePath(QStringLiteral("crash-last.txt")));
    if (!fi.exists())
        return {};
    const QString dest = fi.dir().filePath(QStringLiteral("crash-%1.txt")
                                               .arg(fi.lastModified().toString(QStringLiteral("yyyyMMdd-HHmmss"))));
    QFile::remove(dest);
    return QFile::rename(fi.filePath(), dest) ? dest : fi.filePath();
}

} // namespace CrashReporter
