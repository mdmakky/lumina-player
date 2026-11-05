#include "PlatformIntegration.h"
#include "Mpris.h" // wantsInhibit(): same "playing video" rule as Linux

#include <QMetaObject>
#include <QPointer>

#import <Foundation/Foundation.h>
#import <IOKit/pwr_mgt/IOPMLib.h>
#import <MediaPlayer/MediaPlayer.h>

namespace {

class MacIntegration : public PlatformIntegration
{
public:
    explicit MacIntegration(QObject *parent) : PlatformIntegration(parent)
    {
        MPRemoteCommandCenter *cc = [MPRemoteCommandCenter sharedCommandCenter];
        QPointer<MacIntegration> self(this);
        // Handlers run on a system thread: hop to the Qt thread via queued emits.
        auto hook = [self](MPRemoteCommand *cmd, void (PlatformIntegration::*sig)()) {
            [cmd addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *) {
                if (!self) return MPRemoteCommandHandlerStatusCommandFailed;
                QMetaObject::invokeMethod(self.data(), [self, sig]() { if (self) (self.data()->*sig)(); },
                                          Qt::QueuedConnection);
                return MPRemoteCommandHandlerStatusSuccess;
            }];
        };
        hook(cc.playCommand, &PlatformIntegration::playRequested);
        hook(cc.pauseCommand, &PlatformIntegration::pauseRequested);
        hook(cc.togglePlayPauseCommand, &PlatformIntegration::playPauseRequested);
        hook(cc.stopCommand, &PlatformIntegration::stopRequested);
        hook(cc.nextTrackCommand, &PlatformIntegration::nextRequested);
        hook(cc.previousTrackCommand, &PlatformIntegration::previousRequested);
        [cc.changePlaybackPositionCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *e) {
            const double t = static_cast<MPChangePlaybackPositionCommandEvent *>(e).positionTime;
            if (!self) return MPRemoteCommandHandlerStatusCommandFailed;
            QMetaObject::invokeMethod(self.data(), [self, t]() { if (self) emit self->seekToRequested(t); },
                                      Qt::QueuedConnection);
            return MPRemoteCommandHandlerStatusSuccess;
        }];
    }

    ~MacIntegration() override { setAwake(false); }

    void update(const MediaState &s) override
    {
        setAwake(Mpris::wantsInhibit(s));

        MPNowPlayingInfoCenter *center = [MPNowPlayingInfoCenter defaultCenter];
        if (s.status == MediaState::Stopped && s.url.isEmpty()) {
            center.nowPlayingInfo = nil;
            center.playbackState = MPNowPlayingPlaybackStateStopped;
            return;
        }
        center.nowPlayingInfo = @{
            MPMediaItemPropertyTitle: s.title.toNSString(),
            MPMediaItemPropertyPlaybackDuration: @(s.duration),
            MPNowPlayingInfoPropertyElapsedPlaybackTime: @(s.position),
            MPNowPlayingInfoPropertyPlaybackRate: @(s.status == MediaState::Playing ? s.rate : 0.0),
        };
        center.playbackState = s.status == MediaState::Playing ? MPNowPlayingPlaybackStatePlaying
                                                               : MPNowPlayingPlaybackStatePaused;
        MPRemoteCommandCenter *cc = [MPRemoteCommandCenter sharedCommandCenter];
        cc.nextTrackCommand.enabled = s.canNext;
        cc.previousTrackCommand.enabled = s.canPrevious;
    }

private:
    void setAwake(bool on)
    {
        if (on && m_assertion == kIOPMNullAssertionID) {
            IOPMAssertionCreateWithName(kIOPMAssertionTypePreventUserIdleDisplaySleep, kIOPMAssertionLevelOn,
                                        CFSTR("Lumina Player is playing video"), &m_assertion);
        } else if (!on && m_assertion != kIOPMNullAssertionID) {
            IOPMAssertionRelease(m_assertion);
            m_assertion = kIOPMNullAssertionID;
        }
    }

    IOPMAssertionID m_assertion = kIOPMNullAssertionID;
};

} // namespace

PlatformIntegration *createMacIntegration(QObject *parent) { return new MacIntegration(parent); }
