#include "MpvVideoItem.h"

#include "MpvEngine.h"
#include "utilities/Logger.h"

#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QQuickWindow>
#include <QSGRendererInterface>

#include <mpv/render_gl.h>

namespace {

// mpv needs raw GL function pointers; Qt's QOpenGLContext provides them.
void *getProcAddress(void *ctx, const char *name)
{
    auto *context = static_cast<QOpenGLContext *>(ctx);
    return reinterpret_cast<void *>(context->getProcAddress(QByteArray(name)));
}

mpv_render_context *freeContextAtomically(std::atomic<mpv_render_context *> &slot)
{
    mpv_render_context *ctx = slot.exchange(nullptr);
    if (ctx) {
        mpv_render_context_set_update_callback(ctx, nullptr, nullptr);
        mpv_render_context_free(ctx);
    }
    return ctx;
}

} // namespace

MpvVideoItem::MpvVideoItem(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
{
    // Re-create the render context whenever the item moves to a window with a
    // freshly initialized scene graph.
    connect(this, &QQuickItem::windowChanged, this, [this](QQuickWindow *window) {
    if (window)
        connect(window, &QQuickWindow::sceneGraphInitialized,
                this, &MpvVideoItem::initRenderContext, Qt::DirectConnection);
    });
}

void MpvVideoItem::setEngine(MpvEngine *engine)
{
    if (m_engine == engine)
        return;

    m_engine = engine;
    emit engineChanged();

    // The window (and its scene graph) may already exist when the engine
    // arrives; try immediately, but only if a GL context is actually there —
    // otherwise sceneGraphInitialized will re-invoke us at the right time.
    if (window() && m_engine)
        initRenderContext();
}

MpvVideoItem::~MpvVideoItem()
{
    // The renderer may already have freed the context (render thread teardown
    // with a current GL context); the atomic exchange guarantees a single free.
    freeContextAtomically(m_renderCtx);
}

void MpvVideoItem::initRenderContext()
{
    if (m_renderCtx.load() || !m_engine || !m_engine->handle() || !window())
        return;

    QSGRendererInterface *rendererInterface = window()->rendererInterface();
    auto *glContext = static_cast<QOpenGLContext *>(rendererInterface->getResource(
        window(), QSGRendererInterface::OpenGLContextResource));
    if (!glContext) {
        // Normal during startup: called before the scene graph finished
        // initializing. sceneGraphInitialized re-invokes us when ready.
        qCDebug(lcEngine) << "No OpenGL context yet; deferring render context creation";
        return;
    }

    mpv_opengl_init_params initParams{&getProcAddress, glContext};
    int advancedControl = 0;
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_API_TYPE, const_cast<char *>(MPV_RENDER_API_TYPE_OPENGL)},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &initParams},
        {MPV_RENDER_PARAM_ADVANCED_CONTROL, &advancedControl},
        {},
    };

    mpv_render_context *ctx = nullptr;
    const int rc = mpv_render_context_create(&ctx, m_engine->handle(), params);
    if (rc < 0) {
        qCWarning(lcEngine) << "mpv_render_context_create failed:" << mpv_error_string(rc);
        return;
    }

    m_renderCtx.store(ctx);
    mpv_render_context_set_update_callback(ctx, &MpvVideoItem::mpvUpdateCallback, this);
    qCDebug(lcEngine) << "mpv render context created";
    update();
}

void MpvVideoItem::mpvUpdateCallback(void *ctx)
{
    // Called from mpv's thread: schedule update() on the GUI thread.
    QMetaObject::invokeMethod(static_cast<QQuickFramebufferObject *>(ctx),
                              &QQuickFramebufferObject::update, Qt::QueuedConnection);
}

class MpvRenderer final : public QQuickFramebufferObject::Renderer
{
public:
    explicit MpvRenderer(MpvVideoItem *item)
        : m_item(item)
    {
    }

    ~MpvRenderer() override
    {
        // Runs on the render thread with the GL context current — the ideal
        // place to free the mpv render context if the item hasn't already.
        if (mpv_render_context *ctx = m_item->takeRenderContext()) {
            mpv_render_context_set_update_callback(ctx, nullptr, nullptr);
            mpv_render_context_free(ctx);
        }
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override
    {
        m_size = size;
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        return new QOpenGLFramebufferObject(size, format);
    }

    void render() override
    {
        mpv_render_context *ctx = m_item->renderContext();
        if (!ctx)
            return;

        mpv_opengl_fbo fbo{static_cast<int>(framebufferObject()->handle()),
                           m_size.width(), m_size.height(), /*internal_format=*/0};
        // Qt 6 samples the FBO with GL's bottom-up origin (mirrorVertically defaults
        // to false), which is exactly what mpv renders without flipping.
        // Debug/escape hatch if a platform ever renders upside down: LUMINA_FLIP_Y=1.
        int flipY = qEnvironmentVariableIntValue("LUMINA_FLIP_Y") == 1 ? 1 : 0;
        mpv_render_param params[] = {
            {MPV_RENDER_PARAM_OPENGL_FBO, &fbo},
            {MPV_RENDER_PARAM_FLIP_Y, &flipY},
            {},
        };

        mpv_render_context_render(ctx, params);
    }

    void synchronize(QQuickFramebufferObject *) override {}

private:
    MpvVideoItem *m_item;
    QSize m_size;
};

QQuickFramebufferObject::Renderer *MpvVideoItem::createRenderer() const
{
    return new MpvRenderer(const_cast<MpvVideoItem *>(this));
}
