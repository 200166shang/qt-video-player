#pragma once

namespace playerlab::core {

class AVSynchronizer {
public:
    enum class VideoAction {
        Wait,
        Display,
        Drop,
    };

    struct VideoDecision {
        VideoAction action = VideoAction::Display;
        double diffMs = 0.0;
        int waitMs = 0;
    };

    [[nodiscard]] VideoDecision decideVideoFrame(double videoPtsSec, double masterClockSec) const;

private:
    static constexpr double kSyncThresholdMs = 40.0;
    static constexpr int kMaxWaitMs = 100;
};

}  // namespace playerlab::core
