#pragma once

#include <QObject>
#include <QTranslator>

class QQmlEngine;
class PlayerController;
class ShortcutManager;

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

    // UI language: "system" (default), "en" or "bn". Persisted; switching is
    // live (QQmlEngine::retranslate), no restart. LUMINA_LANG=<code> overrides
    // "system" (self-tests force English with it).
    Q_PROPERTY(QString language READ language NOTIFY languageChanged)
    QString language() const;
    Q_INVOKABLE void setLanguage(const QString &code);

signals:
    void languageChanged();

private:
    void applyLanguage(const QString &code);

    PlayerController *m_player = nullptr;
    QQmlEngine *m_engine = nullptr;
    ShortcutManager *m_shortcuts = nullptr;
    QTranslator m_translator;
};
