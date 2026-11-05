// Lumina Player — application entry point.
//
// Sets up the Qt Quick scene graph backend (OpenGL — required by mpv's render
// API, see ADR-002), installs logging, wires the application context, and
// loads the QML UI from the embedded module.

#include "ApplicationContext.h"
#include "utilities/Logger.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSGRendererInterface>

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

    ApplicationContext context(&app);

    QQmlApplicationEngine engine;
    context.registerOnEngine(&engine);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(1); }, Qt::QueuedConnection);

    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/Lumina/Player/ui/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;

    context.openCommandLineFiles(QGuiApplication::arguments().mid(1));

    if (qEnvironmentVariableIsSet("LUMINA_PLAYBACK_TEST"))
        context.startPlaybackSelfTest();

    return app.exec();
}
