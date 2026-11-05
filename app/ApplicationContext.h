#pragma once

#include <QObject>

class QQmlEngine;
class PlayerController;

// Owns application-level singletons and wires the C++ core to the QML engine.
// Constructed once in main() before the QML engine loads the UI.
class ApplicationContext : public QObject
{
    Q_OBJECT

public:
    explicit ApplicationContext(QObject *parent = nullptr);

    // Registers context properties on the given QML engine's root context.
    void registerOnEngine(QQmlEngine *qmlEngine);

    // Opens media files passed on the command line.
    void openCommandLineFiles(const QStringList &arguments);

    // When LUMINA_PLAYBACK_TEST is set: scripts a load → play → seek → pause →
    // resume cycle, logging observed state each second, then exits. Used as an
    // automated playback smoke test (docs/TESTING.md).
    void startPlaybackSelfTest();

    PlayerController *player() const { return m_player; }

private:
    PlayerController *m_player = nullptr;
};
