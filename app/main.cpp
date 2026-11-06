// Lumina Player — application entry point.
//
// Sets up the Qt Quick scene graph backend (OpenGL — required by mpv's render
// API, see ADR-002), installs logging, wires the application context, and
// loads the QML UI from the embedded module.

#include "ApplicationContext.h"
#include "SingleInstance.h"
#include "UiTest.h"
#include "utilities/Logger.h"
#include "utilities/CrashReporter.h"
#include "services/UpdateChecker.h"

#include <QDir>
#include <QFileInfo>
#include <QFileOpenEvent>
#include <QGuiApplication>
#include <QFileDevice>
#include <QFile>
#include <QSurfaceFormat>
#include <QIcon>
#include <QStandardPaths>
#include <QSettings>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QDateTime>
#include <QTimer>
#include "core/PlayerController.h"
#include <QSGRendererInterface>

namespace {
// macOS delivers Finder "open with" / dock drops as QFileOpenEvent.
class FileOpenFilter : public QObject
{
public:
    explicit FileOpenFilter(ApplicationContext *c) : QObject(c), m_context(c) {}
    bool eventFilter(QObject *, QEvent *event) override
    {
        if (event->type() != QEvent::FileOpen)
            return false;
        const auto *open = static_cast<QFileOpenEvent *>(event);
        m_context->openCommandLineFiles(
            {open->url().isLocalFile() ? open->file() : open->url().toString()});
        return true;
    }

private:
    ApplicationContext *m_context;
};
} // namespace

// Keep history, library and settings private to the user (watch history, file paths).
static void hardenPermissions()
{
#ifndef Q_OS_WIN
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!dataDir.isEmpty()) {
        QDir().mkpath(dataDir);
        QFile::setPermissions(dataDir, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    }
    QSettings settings;
    settings.sync();
    if (QFile::exists(settings.fileName()))
        QFile::setPermissions(settings.fileName(), QFileDevice::ReadOwner | QFileDevice::WriteOwner);
#endif
}

int main(int argc, char *argv[])
{
    QGuiApplication::setOrganizationName(QStringLiteral("mdmakky"));
    // UI/self tests get their own pristine settings/database so runs are repeatable
    // and never touch the user's real history.
    const bool uiTest = qEnvironmentVariableIsSet("LUMINA_UI_TEST")
        || qEnvironmentVariableIsSet("LUMINA_PLAYBACK_TEST");
    QGuiApplication::setApplicationName(uiTest ? QStringLiteral("Lumina Player UITest")
                                               : QStringLiteral("Lumina Player"));
    if (uiTest) {
        QSettings().clear();
        QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).removeRecursively();
    }
    QGuiApplication::setApplicationVersion(QStringLiteral(LUMINA_VERSION));

    // mpv's OpenGL render API requires the Qt scene graph to use OpenGL.
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
#ifdef Q_OS_MACOS
    // macOS defaults to a legacy 2.1 GL context where mpv disables its scalers
    // and zero-copy VideoToolbox; ask for a 3.3 core profile.
    {
        QSurfaceFormat fmt;
        fmt.setVersion(3, 3);
        fmt.setProfile(QSurfaceFormat::CoreProfile);
        QSurfaceFormat::setDefaultFormat(fmt);
    }
#endif
    // The Basic style is unstyled — all visuals come from our Theme system.
    // Set via env so we don't need the QuickControls2 C++ module at build time.
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");

    QGuiApplication app(argc, argv);
    app.setWindowIcon(QIcon(QStringLiteral(":/qt/qml/Lumina/Player/icons/lumina.png")));

    hardenPermissions();
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &hardenPermissions);
    Logger::install();
    qCInfo(lcCore) << "Lumina Player" << LUMINA_VERSION << "starting"
                   << "- Qt" << QT_VERSION_STR;
    const bool testRun = qEnvironmentVariableIsSet("LUMINA_PLAYBACK_TEST") || qEnvironmentVariableIsSet("LUMINA_UI_TEST");
    QString previousCrash;
    if (!testRun) {
        previousCrash = CrashReporter::takePreviousReport(Logger::logsPath());
        CrashReporter::install(Logger::logsPath(), QStringLiteral(LUMINA_VERSION));
    }

    // Second launch hands its files to the running instance and exits
    // (skipped for self-tests so they can run alongside a normal session).
    // Relative paths must be made absolute here: a forwarded argument would otherwise resolve against the
    // running instance's working directory, and fromUserInput() does not treat a bare relative path as a file.
    QStringList cliArgs;
    for (const QString &a : QGuiApplication::arguments().mid(1))
        cliArgs << (QFileInfo::exists(a) ? QFileInfo(a).absoluteFilePath() : a);
    const bool selfTest = qEnvironmentVariableIsSet("LUMINA_PLAYBACK_TEST")
        || qEnvironmentVariableIsSet("LUMINA_UI_TEST")
        || qEnvironmentVariableIsSet("LUMINA_NO_SINGLE_INSTANCE");
    SingleInstance single(QStringLiteral("lumina-player-%1").arg(QDir::home().dirName()));
    if (!selfTest && !UiTest::requested()) {
        if (single.forward(cliArgs)) {
            qCInfo(lcCore) << "Forwarded arguments to running instance";
            return 0;
        }
        single.listen();
    }

    ApplicationContext context(&app);
    app.installEventFilter(new FileOpenFilter(&context));

    UpdateChecker updater{QStringLiteral(LUMINA_VERSION)}; // declared before the engine: it must outlive it (context property)
    QQmlApplicationEngine engine;
    context.registerOnEngine(&engine);
    engine.rootContext()->setContextProperty(QStringLiteral("updater"), &updater);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(1); }, Qt::QueuedConnection);

    engine.addImportPath(QStringLiteral("qrc:/qt/qml")); // Qt 6.4 does not search it by default
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/Lumina/Player/ui/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;

    if (!previousCrash.isEmpty())
        QTimer::singleShot(2500, &app, [&context, previousCrash] {
            context.player()->toast(QCoreApplication::translate("main", "Lumina closed unexpectedly last time. A report was saved in the logs folder."));
            qCWarning(lcCore) << "previous run crashed; report:" << previousCrash;
        });
    if (!testRun && context.player()->setting(QStringLiteral("updates/auto"), false).toString() == QLatin1String("true")) {
        // At most once a day: GitHub's unauthenticated API allows 60 calls/hour per address (shared networks hit it).
        const qint64 now = QDateTime::currentSecsSinceEpoch();
        if (now - context.player()->setting(QStringLiteral("updates/lastCheck"), 0).toLongLong() > 24 * 3600) {
            context.player()->setSetting(QStringLiteral("updates/lastCheck"), now);
            QTimer::singleShot(5000, &updater, &UpdateChecker::check);
        }
    }

    QObject::connect(&single, &SingleInstance::argumentsReceived, &app,
                     [&](const QStringList &files) {
        context.openCommandLineFiles(files);
        if (auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first())) {
            window->show();
            window->raise();
            window->requestActivate();
        }
    });

    // Load only after the first frame is swapped: the mpv render context (VO)
    // exists by then. Loading earlier lets short files hit EOF before any
    // video output is up, so the playlist skips them.
    auto *rootWindow = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    // Hidden QA switch: LUMINA_WINDOW_SIZE=WxH sizes the window (screenshots).
    {
        const QStringList wh = qEnvironmentVariable("LUMINA_WINDOW_SIZE").split('x');
        if (wh.size() == 2 && wh[0].toInt() > 0 && wh[1].toInt() > 0)
            rootWindow->resize(wh[0].toInt(), wh[1].toInt());
    }
    if (UiTest::requested()) {
        // UiTest loads the media itself so it can capture the empty state first.
        (new UiTest(rootWindow, context.player(), cliArgs, &app))->start();
    }
    auto *firstFrame = new QObject(&app);
    QObject::connect(rootWindow, &QQuickWindow::frameSwapped, firstFrame, [&, firstFrame]() {
        firstFrame->deleteLater();
        QObject::disconnect(rootWindow, &QQuickWindow::frameSwapped, firstFrame, nullptr);
        if (!UiTest::requested())
            context.openCommandLineFiles(cliArgs);
    });

    if (selfTest)
        context.startPlaybackSelfTest();

    return app.exec();
}
