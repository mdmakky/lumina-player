#pragma once

#include <QQuickFramebufferObject>
#include <QtQml/qqmlregistration.h>
#include <atomic>

#include "MpvEngine.h"
#include <mpv/render.h>

// The only class that bridges mpv's OpenGL render API and the Qt Quick scene
// graph (architecture rule). mpv renders into Qt's framebuffer object on the
// render thread; the GUI thread is never blocked by video presentation.
//
// See docs/ARCHITECTURE.md and ADR-002 in docs/TECHNICAL_DECISIONS.md.
class MpvVideoItem : public QQuickFramebufferObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(MpvVideoItem)

    Q_PROPERTY(MpvEngine *engine READ engine WRITE setEngine NOTIFY engineChanged FINAL)

public:
    explicit MpvVideoItem(QQuickItem *parent = nullptr);
    ~MpvVideoItem() override;

    Renderer *createRenderer() const override;

    MpvEngine *engine() const { return m_engine; }
    void setEngine(MpvEngine *engine);

signals:
    void engineChanged();

private slots:
    void initRenderContext();

private:
    // Accessors shared with the renderer. Ownership of the raw context is
    // resolved by whoever tears down first (exchange, then free exactly once).
    mpv_render_context *renderContext() const { return m_renderCtx.load(); }
    mpv_render_context *takeRenderContext() { return m_renderCtx.exchange(nullptr); }
    void restoreRenderContext(mpv_render_context *ctx) { m_renderCtx.store(ctx); }

    static void mpvUpdateCallback(void *ctx);

    MpvEngine *m_engine = nullptr;
    std::atomic<mpv_render_context *> m_renderCtx{nullptr};

    friend class MpvRenderer;
};
