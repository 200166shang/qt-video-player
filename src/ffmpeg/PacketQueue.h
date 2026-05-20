#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>

namespace playerlab::ffmpeg {

template <typename T>
class PacketQueue {
public:
    explicit PacketQueue(std::size_t maxSize = 0) : maxSize_(maxSize) {}

    void push(T item) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(item));
        }
        cv_.notify_one();
    }

    bool waitPush(T item) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]() { return abort_ || maxSize_ == 0 || queue_.size() < maxSize_; });
        if (abort_) {
            return false;
        }
        queue_.push(std::move(item));
        lock.unlock();
        cv_.notify_one();
        return true;
    }

    bool waitPop(T& out) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]() { return abort_ || !queue_.empty(); });
        if (abort_) {
            return false;
        }
        out = std::move(queue_.front());
        queue_.pop();
        lock.unlock();
        cv_.notify_one();
        return true;
    }

    bool tryPop(T& out) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return false;
        }
        out = std::move(queue_.front());
        queue_.pop();
        cv_.notify_one();
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
        cv_.notify_all();
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        abort_ = false;
        queue_ = {};
        cv_.notify_all();
    }

    void setMaxSize(std::size_t maxSize) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            maxSize_ = maxSize;
        }
        cv_.notify_all();
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
    std::size_t maxSize_ = 0;
    bool abort_ = false;
};

}  // namespace playerlab::ffmpeg
