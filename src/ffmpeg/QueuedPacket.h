#pragma once

struct AVPacket;

namespace playerlab::ffmpeg {

enum class QueuedPacketKind {
    Data,
    Flush,
    Eof,
};

struct QueuedPacket {
    AVPacket* packet = nullptr;
    QueuedPacketKind kind = QueuedPacketKind::Data;
    int serial = 0;
};

}  // namespace playerlab::ffmpeg
