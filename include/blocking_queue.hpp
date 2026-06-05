#pragma once

#include <mutex>
#include <optional>
#include <queue>
#include <utility>

namespace mt {

template <typename T>
class BlockingQueue {
public:
    BlockingQueue() = default;

    BlockingQueue(const BlockingQueue&) = delete;
    BlockingQueue& operator=(const BlockingQueue&) = delete;

    void push(T value)
    {
        const std::lock_guard lock{mutex_};
        queue_.push(std::move(value));
    }

    [[nodiscard]] std::optional<T> wait_pop()
    {
        const std::lock_guard lock{mutex_};
        if (queue_.empty()) {
            return std::nullopt;
        }

        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

    void close() noexcept
    {
        const std::lock_guard lock{mutex_};
        closed_ = true;
    }

    [[nodiscard]] bool is_closed() const
    {
        const std::lock_guard lock{mutex_};
        return closed_;
    }

private:
    mutable std::mutex mutex_;
    std::queue<T> queue_;
    bool closed_{false};
};

} // namespace mt
