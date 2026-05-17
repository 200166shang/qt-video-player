#pragma once

#include <chrono>

namespace playerlab::core {

class PlaybackClock {
public:
    void reset(bool hasAudio);
    void onPauseChanged(bool paused);
    void updateAudioClock(double clockSec);
    void ensureSystemClockStarted(double startPtsSec);
    [[nodiscard]] double masterClockSec() const;

private:
    using Clock = std::chrono::steady_clock;

    [[nodiscard]] static double secondsBetween(Clock::time_point from, Clock::time_point to);

    bool hasAudio_ = false;
    bool paused_ = false;

    bool systemClockStarted_ = false;
    double systemBasePtsSec_ = 0.0;
    Clock::time_point systemBaseWall_{};

    bool audioClockValid_ = false;
    double audioAnchorPtsSec_ = 0.0;
    Clock::time_point audioAnchorWall_{};

    Clock::time_point pauseWall_{};
};

}  // namespace playerlab::core
