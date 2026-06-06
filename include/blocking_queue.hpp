#pragma once

#include <condition_variable>
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
        {
            const std::lock_guard lock{mutex_};
            queue_.push(std::move(value));
        }
        condition_.notify_one();
    }

    [[nodiscard]] std::optional<T> wait_pop()
    {
        std::unique_lock lock{mutex_};
        condition_.wait(lock, [this] {
            return closed_ || !queue_.empty();
        });

        if (queue_.empty()) {
            return std::nullopt;
        }

        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

    void close() noexcept
    {
        {
            const std::lock_guard lock{mutex_};
            closed_ = true;
        }
        condition_.notify_all();
    }

    [[nodiscard]] bool is_closed() const
    {
        const std::lock_guard lock{mutex_};
        return closed_;
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::queue<T> queue_;
    bool closed_{false};
};

} // namespace mt
