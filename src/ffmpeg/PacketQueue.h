#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>

namespace playerlab::ffmpeg {

template <typename T>
class PacketQueue {
public:
    void push(T item) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(item));
        }
        cv_.notify_one();
    }

    bool waitPop(T& out) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]() { return abort_ || !queue_.empty(); });
        if (abort_) {
            return false;
        }
        out = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    bool tryPop(T& out) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return false;
        }
        out = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    void abort() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            abort_ = true;
        }
        cv_.notify_all();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_ = {};
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        abort_ = false;
        queue_ = {};
    }

    [[nodiscard]] bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    [[nodiscard]] std::size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<T> queue_;
    bool abort_ = false;
};

}  // namespace playerlab::ffmpeg
