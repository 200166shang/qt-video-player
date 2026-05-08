#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace playerlab::core {

struct VideoFrame {
    int width = 0;
    int height = 0;
    std::array<int, 3> linesize = {0, 0, 0};
    std::array<std::vector<std::uint8_t>, 3> planes;
    double ptsSec = 0.0;

    [[nodiscard]] bool isValidYuv420p() const {
        return width > 0 && height > 0 && !planes[0].empty() && !planes[1].empty() && !planes[2].empty();
    }
};

}  // namespace playerlab::core
