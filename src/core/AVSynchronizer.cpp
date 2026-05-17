#include "core/AVSynchronizer.h"

#include <algorithm>
#include <cmath>

namespace playerlab::core {

AVSynchronizer::VideoDecision AVSynchronizer::decideVideoFrame(const double videoPtsSec, const double masterClockSec) const {
    VideoDecision decision;
    decision.diffMs = (videoPtsSec - masterClockSec) * 1000.0;

    if (decision.diffMs > kSyncThresholdMs) {
        decision.action = VideoAction::Wait;
        const int waitMs = static_cast<int>(std::lround(decision.diffMs));
        decision.waitMs = std::clamp(waitMs, 1, kMaxWaitMs);
        return decision;
    }

    if (decision.diffMs < -kSyncThresholdMs) {
        decision.action = VideoAction::Drop;
        return decision;
    }

    decision.action = VideoAction::Display;
    return decision;
}

}  // namespace playerlab::core
