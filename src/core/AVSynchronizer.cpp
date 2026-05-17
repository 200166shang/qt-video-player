#include "core/AVSynchronizer.h"

#include <algorithm>

namespace playerlab::core {

double AVSynchronizer::computeTargetDelay(const double baseDelaySec, const double videoPtsSec,
                                          const double masterClockSec) const {
    const double safeBaseDelay = std::clamp(baseDelaySec, kMinDelaySec, kMaxDelaySec);
    const double diffSec = videoPtsSec - masterClockSec;

    // Tiny drift: keep cadence stable, avoid boundary oscillation.
    if (std::abs(diffSec) <= kNoAdjustThresholdSec) {
        return safeBaseDelay;
    }

    if (diffSec > kSyncThresholdSec) {
        return std::clamp(safeBaseDelay + diffSec, kMinDelaySec, kMaxDelaySec);
    }

    if (diffSec < -kSyncThresholdSec) {
        return kMinDelaySec;
    }

    const double adjustedDelay = safeBaseDelay + diffSec;
    return std::clamp(adjustedDelay, kMinDelaySec, kMaxDelaySec);
}

}  // namespace playerlab::core
