#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>

#include "engine/MpvEngine.h"

// Application Core layer: the single object QML talks to. Holds playback
// state that is presentation-relevant and forwards commands to the media
// engine. Must not expose mpv types or contain playback logic itself.
class PlayerController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(MpvEngine *engine READ engine CONSTANT FINAL)
    Q_PROPERTY(bool hasMedia READ hasMedia NOTIFY hasMediaChanged FINAL)
    Q_PROPERTY(QStringList recentFiles READ recentFiles NOTIFY recentFilesChanged FINAL)

public:
    explicit PlayerController(QObject *parent = nullptr);

    MpvEngine *engine() const { return m_engine; }
    bool hasMedia() const;
    QStringList recentFiles() const { return m_recentFiles; }

public slots:
    // Accepts a JS array of URLs (from file dialogs or drag & drop).
    void openFiles(const QVariantList &urls);
    void clearRecentFiles();

signals:
    void hasMediaChanged();
    void recentFilesChanged();

private:
    void rememberFile(const QUrl &url);

    MpvEngine *m_engine = nullptr;
    QStringList m_recentFiles;
};
