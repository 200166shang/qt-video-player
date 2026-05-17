#include "core/PlaybackClock.h"

namespace playerlab::core {

void PlaybackClock::reset(const bool hasAudio) {
    hasAudio_ = hasAudio;
    paused_ = false;

    systemClockStarted_ = false;
    systemBasePtsSec_ = 0.0;
    systemBaseWall_ = {};

    audioClockValid_ = false;
    audioAnchorPtsSec_ = 0.0;
    audioAnchorWall_ = {};

    pauseWall_ = {};
    playbackRate_ = 1.0;
}

void PlaybackClock::onPauseChanged(const bool paused) {
    if (paused_ == paused) {
        return;
    }

    if (paused) {
        pauseWall_ = Clock::now();
        paused_ = true;
        return;
    }

    const Clock::time_point now = Clock::now();
    const Clock::duration pauseDuration = now - pauseWall_;
    if (systemClockStarted_) {
        systemBaseWall_ += pauseDuration;
    }
    if (audioClockValid_) {
        audioAnchorWall_ += pauseDuration;
    }
    paused_ = false;
}

void PlaybackClock::updateAudioClock(const double clockSec) {
    audioAnchorPtsSec_ = clockSec;
    audioAnchorWall_ = paused_ ? pauseWall_ : Clock::now();
    audioClockValid_ = true;
}

void PlaybackClock::ensureSystemClockStarted(const double startPtsSec) {
    if (systemClockStarted_) {
        return;
    }
    systemBasePtsSec_ = startPtsSec;
    systemBaseWall_ = paused_ ? pauseWall_ : Clock::now();
    systemClockStarted_ = true;
}

void PlaybackClock::setPlaybackRate(const double rate) {
    const double safeRate = rate > 0.0 ? rate : 1.0;
    if (playbackRate_ == safeRate) {
        return;
    }

    const Clock::time_point now = paused_ ? pauseWall_ : Clock::now();
    const double currentMaster = masterClockSec();
    playbackRate_ = safeRate;

    if (hasAudio_ && audioClockValid_) {
        audioAnchorPtsSec_ = currentMaster;
        audioAnchorWall_ = now;
        return;
    }

    if (systemClockStarted_) {
        systemBasePtsSec_ = currentMaster;
        systemBaseWall_ = now;
    }
}

double PlaybackClock::masterClockSec() const {
    const Clock::time_point now = paused_ ? pauseWall_ : Clock::now();

    if (hasAudio_ && audioClockValid_) {
        return audioAnchorPtsSec_ + secondsBetween(audioAnchorWall_, now);
    }

    if (!systemClockStarted_) {
        return 0.0;
    }
    return systemBasePtsSec_ + secondsBetween(systemBaseWall_, now) * playbackRate_;
}

double PlaybackClock::secondsBetween(const Clock::time_point from, const Clock::time_point to) {
    return std::chrono::duration<double>(to - from).count();
}

}  // namespace playerlab::core
