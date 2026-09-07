#pragma once

#include <array>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <utility>

namespace playerlab::ffmpeg {

/// Thread-safe bounded ring buffer for decoded frames (VideoFrame / AudioFrame).
///
/// Producer (decoder thread) back-pressures when full via blocking push().
/// Consumer (main thread timer) uses non-blocking tryPop().
/// Two condition variables avoid spurious wake-ups between producer and consumer.
template <typename T, std::size_t Capacity = 16>
class CircularFrameQueue {
    static_assert(Capacity > 1);

public:
    struct Metrics {
        std::size_t totalPushed = 0;
        std::size_t totalPopped = 0;
        std::size_t totalDropped = 0;
        std::size_t highWaterMark = 0;
        std::size_t currentSize = 0;
    };

    CircularFrameQueue() = default;

    CircularFrameQueue(const CircularFrameQueue&) = delete;
    CircularFrameQueue& operator=(const CircularFrameQueue&) = delete;

    // --- Producer API (decoder thread) ---

    /// Block until a slot is available, then push |item|.
    /// Returns false immediately if the queue has been aborted (shutdown path).
    [[nodiscard]] bool push(T item) {
        std::unique_lock<std::mutex> lock(mutex_);
        notFullCv_.wait(lock, [this]() { return abort_ || count_ < Capacity; });
        if (abort_) {
            return false;
        }

        buffer_[writeIdx_].emplace(std::move(item));
        writeIdx_ = advance(writeIdx_);
        ++count_;
        ++totalPushed_;
        if (count_ > highWaterMark_) {
            highWaterMark_ = count_;
        }

        lock.unlock();
        notEmptyCv_.notify_one();
        return true;
    }

    /// Non-blocking push. Silently drops |item| if the queue is full.
    /// Returns true if pushed, false if full or aborted.
    [[nodiscard]] bool tryPush(T item) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (abort_ || count_ >= Capacity) {
            if (!abort_) {
                ++totalDropped_;
            }
            return false;
        }

        buffer_[writeIdx_].emplace(std::move(item));
        writeIdx_ = advance(writeIdx_);
        ++count_;
        ++totalPushed_;
        if (count_ > highWaterMark_) {
            highWaterMark_ = count_;
        }

        notEmptyCv_.notify_one();
        return true;
    }

    // --- Consumer API (main thread) ---

    /// Non-blocking pop. Returns false if empty.
    [[nodiscard]] bool tryPop(T& out) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (count_ == 0) {
            return false;
        }

        out = std::move(*buffer_[readIdx_]);
        buffer_[readIdx_].reset();
        readIdx_ = advance(readIdx_);
        --count_;
        ++totalPopped_;

        notFullCv_.notify_one();
        return true;
    }

    // --- Control ---

    /// Wake all blocked waiters. Subsequent push() returns false.
    /// Paired with reset() before re-use after stop.
    void abort() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            abort_ = true;
        }
        notFullCv_.notify_all();
        notEmptyCv_.notify_all();
    }

    /// Reset to initial (pre-abort) state. Call after stop() + before open().
    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& slot : buffer_) {
            slot.reset();
        }
        writeIdx_ = 0;
        readIdx_ = 0;
        count_ = 0;
        abort_ = false;
        totalPushed_ = 0;
        totalPopped_ = 0;
        totalDropped_ = 0;
        highWaterMark_ = 0;
    }

    /// Drain all items without cleanup (used during stop after thread joined).
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (count_ > 0) {
            buffer_[readIdx_].reset();
            readIdx_ = advance(readIdx_);
            --count_;
        }
        writeIdx_ = 0;
        readIdx_ = 0;
        notFullCv_.notify_all();
    }

    // --- State queries ---

    [[nodiscard]] bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return count_ == 0;
    }

    [[nodiscard]] std::size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return count_;
    }

    [[nodiscard]] static constexpr std::size_t capacity() {
        return Capacity;
    }

    [[nodiscard]] Metrics snapshotMetrics() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return Metrics{
            .totalPushed = totalPushed_,
            .totalPopped = totalPopped_,
            .totalDropped = totalDropped_,
            .highWaterMark = highWaterMark_,
            .currentSize = count_,
        };
    }

private:
    [[nodiscard]] static std::size_t advance(const std::size_t idx) {
        return (idx + 1) % Capacity;
    }

    std::array<std::optional<T>, Capacity> buffer_{};
    std::size_t writeIdx_ = 0;
    std::size_t readIdx_ = 0;
    std::size_t count_ = 0;

    mutable std::mutex mutex_;
    std::condition_variable notFullCv_;
    std::condition_variable notEmptyCv_;
    bool abort_ = false;

    std::size_t totalPushed_ = 0;
    std::size_t totalPopped_ = 0;
    std::size_t totalDropped_ = 0;
    std::size_t highWaterMark_ = 0;
};

}  // namespace playerlab::ffmpeg
