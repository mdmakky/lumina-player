#include "PlatformIntegration.h"

#if defined(Q_OS_LINUX)
#include "MprisIntegration.h"
#elif defined(Q_OS_MACOS)
PlatformIntegration *createMacIntegration(QObject *parent); // MacIntegration.mm
#else
namespace {
// Windows / BSD: no backend yet.
class NullIntegration : public PlatformIntegration
{
public:
    using PlatformIntegration::PlatformIntegration;
    void update(const MediaState &) override {}
};
} // namespace
#endif

PlatformIntegration *PlatformIntegration::create(QObject *parent)
{
#if defined(Q_OS_LINUX)
    return new MprisIntegration(parent);
#elif defined(Q_OS_MACOS)
    return createMacIntegration(parent);
#else
    return new NullIntegration(parent);
#endif
}
