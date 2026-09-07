#include "ffmpeg/FFmpegReadWorker.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

#include <limits>

#include "utils/Logger.h"

namespace {

std::string ffmpegErrorToString(const int errNum) {
    char errBuf[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(errNum, errBuf, sizeof(errBuf));
    return std::string(errBuf);
}

}  // namespace

namespace playerlab::ffmpeg {

FFmpegReadWorker::~FFmpegReadWorker() {
    stop();
}

bool FFmpegReadWorker::open(const playerlab::core::MediaSource& source, playerlab::core::MediaInfo& outInfo,
                            std::string& outError, const double startPositionSec) {
    stop();

    const int openRet = avformat_open_input(&formatContext_, source.uri.c_str(), nullptr, nullptr);
    if (openRet < 0) {
        outError = "open input failed: " + ffmpegErrorToString(openRet);
        return false;
    }

    const int streamRet = avformat_find_stream_info(formatContext_, nullptr);
    if (streamRet < 0) {
        outError = "find stream info failed: " + ffmpegErrorToString(streamRet);
        stop();
        return false;
    }

    videoStreamIndex_ = av_find_best_stream(formatContext_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    audioStreamIndex_ = av_find_best_stream(formatContext_, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (videoStreamIndex_ < 0 && audioStreamIndex_ < 0) {
        outError = "no playable audio/video stream found";
        stop();
        return false;
    }

    if (startPositionSec > 0.0 && !seekTo(startPositionSec, outError)) {
        stop();
        return false;
    }

    outInfo = buildMediaInfo(source, formatContext_);
    readFinished_.store(false);
    currentSerial_.store(1);
    eofSerial_.store(-1);
    if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
        LOG_DEBUG("Read worker open: source='{}' videoStream={} audioStream={} seek={:.3f}", source.uri,
                  videoStreamIndex_, audioStreamIndex_, startPositionSec);
    }
    return true;
}

void FFmpegReadWorker::start(PacketQueue<QueuedPacket>* videoPacketQueue, PacketQueue<QueuedPacket>* audioPacketQueue) {
    videoPacketQueue_ = videoPacketQueue;
    audioPacketQueue_ = audioPacketQueue;
    readFinished_.store(false);
    running_.store(true);
    currentSerial_.store(1);
    eofSerial_.store(-1);
    if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
        LOG_DEBUG("Read worker start: videoQ={} audioQ={}", videoPacketQueue_ != nullptr, audioPacketQueue_ != nullptr);
    }
    readThread_ = std::thread([this]() { readLoop(); });
}

void FFmpegReadWorker::stop() {
    running_.store(false);
    seekCv_.notify_all();

    if (readThread_.joinable()) {
        readThread_.join();
    }

    videoPacketQueue_ = nullptr;
    audioPacketQueue_ = nullptr;
    if (formatContext_ != nullptr) {
        avformat_close_input(&formatContext_);
    }
    videoStreamIndex_ = -1;
    audioStreamIndex_ = -1;
    readFinished_.store(false);
    currentSerial_.store(1);
    eofSerial_.store(-1);
    {
        std::lock_guard<std::mutex> lock(seekMutex_);
        pendingSeekRequest_.reset();
        pendingSeekSerial_ = 0;
    }
    if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
        LOG_DEBUG("Read worker stop");
    }
}

int FFmpegReadWorker::requestSeek(const SeekRequest& request) {
    int reservedSerial = 0;
    {
        std::lock_guard<std::mutex> lock(seekMutex_);
        if (!pendingSeekRequest_.has_value()) {
            pendingSeekSerial_ = currentSerial_.load() + 1;
        }
        pendingSeekRequest_ = request;
        reservedSerial = pendingSeekSerial_;
    }
    if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
        LOG_DEBUG("Read worker seek requested: target={:.3f} rel={:.3f} flags={}", request.targetSec, request.relSec,
                  request.flags);
    }
    seekCv_.notify_all();
    return reservedSerial;
}

const AVCodecParameters* FFmpegReadWorker::videoCodecParameters() const {
    if (formatContext_ == nullptr || videoStreamIndex_ < 0) {
        return nullptr;
    }
    return formatContext_->streams[videoStreamIndex_]->codecpar;
}

const AVCodecParameters* FFmpegReadWorker::audioCodecParameters() const {
    if (formatContext_ == nullptr || audioStreamIndex_ < 0) {
        return nullptr;
    }
    return formatContext_->streams[audioStreamIndex_]->codecpar;
}

AVRational FFmpegReadWorker::videoTimeBase() const {
    if (formatContext_ == nullptr || videoStreamIndex_ < 0) {
        return AVRational{0, 1};
    }
    return formatContext_->streams[videoStreamIndex_]->time_base;
}

AVRational FFmpegReadWorker::audioTimeBase() const {
    if (formatContext_ == nullptr || audioStreamIndex_ < 0) {
        return AVRational{0, 1};
    }
    return formatContext_->streams[audioStreamIndex_]->time_base;
}

bool FFmpegReadWorker::seekTo(const double startPositionSec, std::string& outError) {
    const int64_t targetPts = static_cast<int64_t>(startPositionSec * AV_TIME_BASE);
    const int seekRet =
        avformat_seek_file(formatContext_, -1, std::numeric_limits<int64_t>::min(), targetPts,
                           std::numeric_limits<int64_t>::max(), AVSEEK_FLAG_BACKWARD);
    if (seekRet < 0) {
        outError = "seek input failed: " + ffmpegErrorToString(seekRet);
        return false;
    }
    avformat_flush(formatContext_);
    return true;
}

bool FFmpegReadWorker::performPendingSeek() {
    std::optional<SeekRequest> request;
    int nextSerial = 0;
    {
        std::lock_guard<std::mutex> lock(seekMutex_);
        if (!pendingSeekRequest_.has_value()) {
            return false;
        }
        request = pendingSeekRequest_;
        pendingSeekRequest_.reset();
        nextSerial = pendingSeekSerial_;
        pendingSeekSerial_ = 0;
    }

    if (!request.has_value() || formatContext_ == nullptr) {
        return false;
    }

    const int64_t targetPts = static_cast<int64_t>(request->targetSec * AV_TIME_BASE);
    const int seekRet =
        avformat_seek_file(formatContext_, -1, std::numeric_limits<int64_t>::min(), targetPts,
                           std::numeric_limits<int64_t>::max(), request->flags);
    if (seekRet < 0) {
        if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
            LOG_WARN("Read worker seek failed: target={:.3f} err={}", request->targetSec, ffmpegErrorToString(seekRet));
        }
        return true;
    }

    avformat_flush(formatContext_);

    currentSerial_.store(nextSerial);
    eofSerial_.store(-1);

    if (videoPacketQueue_ != nullptr) {
        videoPacketQueue_->clearWith(freeQueuedPacket);
        videoPacketQueue_->waitPush(
            QueuedPacket{.packet = nullptr, .kind = QueuedPacketKind::Flush, .serial = nextSerial});
    }
    if (audioPacketQueue_ != nullptr) {
        audioPacketQueue_->clearWith(freeQueuedPacket);
        audioPacketQueue_->waitPush(
            QueuedPacket{.packet = nullptr, .kind = QueuedPacketKind::Flush, .serial = nextSerial});
    }

    if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
        LOG_DEBUG("Read worker seek applied: target={:.3f} serial={}", request->targetSec, nextSerial);
    }
    return true;
}

void FFmpegReadWorker::waitForSeekRequest() {
    std::unique_lock<std::mutex> lock(seekMutex_);
    seekCv_.wait(lock, [this]() { return !running_.load() || pendingSeekRequest_.has_value(); });
}

void FFmpegReadWorker::readLoop() {
    std::uint64_t videoPackets = 0;
    std::uint64_t audioPackets = 0;
    bool reachedEof = false;

    while (running_.load()) {
        if (performPendingSeek()) {
            reachedEof = false;
            continue;
        }

        AVPacket* packet = av_packet_alloc();
        if (packet == nullptr) {
            break;
        }

        const int readRet = av_read_frame(formatContext_, packet);
        if (readRet < 0) {
            av_packet_free(&packet);
            if (readRet == AVERROR_EOF) {
                reachedEof = true;
                const int eofSerial = currentSerial_.load();
                if (videoPacketQueue_ != nullptr && videoStreamIndex_ >= 0) {
                    videoPacketQueue_->waitPush(
                        QueuedPacket{.packet = nullptr, .kind = QueuedPacketKind::Eof, .serial = eofSerial});
                }
                if (audioPacketQueue_ != nullptr && audioStreamIndex_ >= 0) {
                    audioPacketQueue_->waitPush(
                        QueuedPacket{.packet = nullptr, .kind = QueuedPacketKind::Eof, .serial = eofSerial});
                }
                eofSerial_.store(eofSerial);
                waitForSeekRequest();
                continue;
            }
            break;
        }

        PacketQueue<QueuedPacket>* targetQueue = nullptr;
        if (packet->stream_index == videoStreamIndex_) {
            targetQueue = videoPacketQueue_;
            ++videoPackets;
        } else if (packet->stream_index == audioStreamIndex_) {
            targetQueue = audioPacketQueue_;
            ++audioPackets;
        }

        if (targetQueue == nullptr) {
            av_packet_free(&packet);
            continue;
        }

        if (!targetQueue->waitPush(
                QueuedPacket{.packet = packet, .kind = QueuedPacketKind::Data, .serial = currentSerial_.load()})) {
            av_packet_free(&packet);
            break;
        }

        if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
            const auto totalPackets = videoPackets + audioPackets;
            if (totalPackets > 0 && (totalPackets % 120) == 0) {
                LOG_TRACE("Read worker dispatch: videoPackets={} audioPackets={} videoQ={} audioQ={}", videoPackets,
                          audioPackets, videoPacketQueue_ != nullptr ? videoPacketQueue_->size() : 0,
                          audioPacketQueue_ != nullptr ? audioPacketQueue_->size() : 0);
            }
        }
    }

    readFinished_.store(true);
    if (playerlab::utils::Logger::isPipelineDebugEnabled()) {
        LOG_DEBUG("Read worker finished: eof={} videoPackets={} audioPackets={} videoQ={} audioQ={}", reachedEof,
                  videoPackets, audioPackets, videoPacketQueue_ != nullptr ? videoPacketQueue_->size() : 0,
                  audioPacketQueue_ != nullptr ? audioPacketQueue_->size() : 0);
    }
}

playerlab::core::MediaInfo FFmpegReadWorker::buildMediaInfo(const playerlab::core::MediaSource& source,
                                                            const AVFormatContext* formatContext) {
    playerlab::core::MediaInfo info;
    info.filePath = source.uri;
    info.containerFormat = formatContext->iformat != nullptr ? formatContext->iformat->long_name : "unknown";
    if (formatContext->duration > 0) {
        info.durationMs = formatContext->duration / (AV_TIME_BASE / 1000);
    }

    for (unsigned int i = 0; i < formatContext->nb_streams; ++i) {
        AVStream* stream = formatContext->streams[i];
        const AVCodecParameters* codecPar = stream->codecpar;
        const AVCodecDescriptor* codecDesc = avcodec_descriptor_get(codecPar->codec_id);
        const std::string codecName = codecDesc != nullptr ? codecDesc->name : "unknown";

        if (codecPar->codec_type == AVMEDIA_TYPE_VIDEO && !info.hasVideo) {
            info.hasVideo = true;
            info.videoWidth = codecPar->width;
            info.videoHeight = codecPar->height;
            info.videoCodec = codecName;
            const AVRational fpsR = av_guess_frame_rate(const_cast<AVFormatContext*>(formatContext), stream, nullptr);
            if (fpsR.den != 0 && fpsR.num != 0) {
                info.frameRate = static_cast<double>(fpsR.num) / static_cast<double>(fpsR.den);
            }
            continue;
        }

        if (codecPar->codec_type == AVMEDIA_TYPE_AUDIO && !info.hasAudio) {
            info.hasAudio = true;
            info.audioCodec = codecName;
            info.audioSampleRate = codecPar->sample_rate;
            info.audioChannels = codecPar->ch_layout.nb_channels;
        }
    }

    return info;
}

void FFmpegReadWorker::freeQueuedPacket(QueuedPacket& queuedPacket) {
    if (queuedPacket.packet != nullptr) {
        av_packet_free(&queuedPacket.packet);
    }
    queuedPacket.packet = nullptr;
}

}  // namespace playerlab::ffmpeg
