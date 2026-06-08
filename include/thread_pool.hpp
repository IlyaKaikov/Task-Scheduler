#pragma once

#include "blocking_queue.hpp"

#include <atomic>
#include <cstddef>
#include <functional>
#include <thread>
#include <vector>

namespace mt {

class ThreadPool {
public:
    explicit ThreadPool(std::size_t worker_count);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    void submit(std::function<void()> task);
    void shutdown();

    [[nodiscard]] std::size_t worker_count() const noexcept;
    [[nodiscard]] bool is_shutdown() const noexcept;

private:
    BlockingQueue<std::function<void()>> tasks_;
    std::vector<std::jthread> workers_;
    std::atomic_bool shutdown_started_{false};
    std::size_t worker_count_;
};

} // namespace mt
