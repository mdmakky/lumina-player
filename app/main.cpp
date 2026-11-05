// Lumina Player — application entry point.
//
// Sets up the Qt Quick scene graph backend (OpenGL — required by mpv's render
// API, see ADR-002), installs logging, wires the application context, and
// loads the QML UI from the embedded module.

#include "ApplicationContext.h"
#include "SingleInstance.h"
#include "UiTest.h"
#include "utilities/Logger.h"

#include <QDir>
#include <QFileOpenEvent>
#include <QGuiApplication>
#include <QIcon>
#include <QStandardPaths>
#include <QSettings>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
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
    // The Basic style is unstyled — all visuals come from our Theme system.
    // Set via env so we don't need the QuickControls2 C++ module at build time.
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");

    QGuiApplication app(argc, argv);
    app.setWindowIcon(QIcon(QStringLiteral(":/qt/qml/Lumina/Player/icons/lumina.png")));

    Logger::install();
    qCInfo(lcCore) << "Lumina Player" << LUMINA_VERSION << "starting"
                   << "- Qt" << QT_VERSION_STR;

    // Second launch hands its files to the running instance and exits
    // (skipped for self-tests so they can run alongside a normal session).
    const bool selfTest = qEnvironmentVariableIsSet("LUMINA_PLAYBACK_TEST")
        || qEnvironmentVariableIsSet("LUMINA_UI_TEST")
        || qEnvironmentVariableIsSet("LUMINA_NO_SINGLE_INSTANCE");
    SingleInstance single(QStringLiteral("lumina-player-%1").arg(QDir::home().dirName()));
    if (!selfTest && !UiTest::requested()) {
        if (single.forward(QGuiApplication::arguments().mid(1))) {
            qCInfo(lcCore) << "Forwarded arguments to running instance";
            return 0;
        }
        single.listen();
    }

    ApplicationContext context(&app);
    app.installEventFilter(new FileOpenFilter(&context));

    QQmlApplicationEngine engine;
    context.registerOnEngine(&engine);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(1); }, Qt::QueuedConnection);

    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/Lumina/Player/ui/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;

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
        (new UiTest(rootWindow, context.player(), QGuiApplication::arguments().mid(1), &app))->start();
    }
    auto *firstFrame = new QObject(&app);
    QObject::connect(rootWindow, &QQuickWindow::frameSwapped, firstFrame, [&, firstFrame]() {
        firstFrame->deleteLater();
        QObject::disconnect(rootWindow, &QQuickWindow::frameSwapped, firstFrame, nullptr);
        if (!UiTest::requested())
            context.openCommandLineFiles(QGuiApplication::arguments().mid(1));
    });

    if (selfTest)
        context.startPlaybackSelfTest();

    return app.exec();
}
