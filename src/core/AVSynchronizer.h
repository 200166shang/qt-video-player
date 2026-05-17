#pragma once

namespace playerlab::core {

class AVSynchronizer {
public:
    [[nodiscard]] double computeTargetDelay(double baseDelaySec, double videoPtsSec, double masterClockSec) const;

private:
    static constexpr double kMinDelaySec = 0.004;
    static constexpr double kMaxDelaySec = 0.100;
    static constexpr double kNoAdjustThresholdSec = 0.010;
    static constexpr double kSyncThresholdSec = 0.100;
};

}  // namespace playerlab::core
