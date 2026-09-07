#pragma once

#include "ffmpeg/CircularFrameQueue.h"

// Transitional alias: new code should use CircularFrameQueue directly.
// FrameQueue<T> was previously a using alias for PacketQueue<T> (unbounded).
// CircularFrameQueue is a bounded ring buffer with back-pressure.
namespace playerlab::ffmpeg {

template <typename T, std::size_t Capacity = 16>
using FrameQueue = CircularFrameQueue<T, Capacity>;

}  // namespace playerlab::ffmpeg
