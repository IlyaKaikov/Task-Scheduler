#pragma once

#include <mutex>
#include <optional>

namespace mt {

template <typename T>
class BlockingQueue {
public:
    BlockingQueue() = default;

    BlockingQueue(const BlockingQueue&) = delete;
    BlockingQueue& operator=(const BlockingQueue&) = delete;

    void push(T value)
    {
        (void)value;
    }

    [[nodiscard]] std::optional<T> wait_pop()
    {
        return std::nullopt;
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
    bool closed_{false};
};

} // namespace mt
