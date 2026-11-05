// Lumina Player — application entry point.
//
// Sets up the Qt Quick scene graph backend (OpenGL — required by mpv's render
// API, see ADR-002), installs logging, wires the application context, and
// loads the QML UI from the embedded module.

#include "ApplicationContext.h"
#include "SingleInstance.h"
#include "utilities/Logger.h"

#include <QDir>
#include <QFileOpenEvent>
#include <QGuiApplication>
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
    QGuiApplication::setApplicationName(QStringLiteral("Lumina Player"));
    QGuiApplication::setApplicationVersion(QStringLiteral(LUMINA_VERSION));

    // mpv's OpenGL render API requires the Qt scene graph to use OpenGL.
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    // The Basic style is unstyled — all visuals come from our Theme system.
    // Set via env so we don't need the QuickControls2 C++ module at build time.
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");

    QGuiApplication app(argc, argv);

    Logger::install();
    qCInfo(lcCore) << "Lumina Player" << LUMINA_VERSION << "starting"
                   << "- Qt" << QT_VERSION_STR;

    // Second launch hands its files to the running instance and exits
    // (skipped for self-tests so they can run alongside a normal session).
    const bool selfTest = qEnvironmentVariableIsSet("LUMINA_PLAYBACK_TEST");
    SingleInstance single(QStringLiteral("lumina-player-%1").arg(QDir::home().dirName()));
    if (!selfTest) {
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

    context.openCommandLineFiles(QGuiApplication::arguments().mid(1));

    if (selfTest)
        context.startPlaybackSelfTest();

    return app.exec();
}
