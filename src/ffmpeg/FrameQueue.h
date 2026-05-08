#pragma once

#include "ffmpeg/PacketQueue.h"

namespace playerlab::ffmpeg {

template <typename T>
using FrameQueue = PacketQueue<T>;

}  // namespace playerlab::ffmpeg
