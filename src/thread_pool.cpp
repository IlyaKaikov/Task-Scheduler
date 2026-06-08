#include "thread_pool.hpp"

#include <stdexcept>

namespace mt {

ThreadPool::ThreadPool(std::size_t worker_count)
    : worker_count_(worker_count)
{
    workers_.reserve(worker_count_);
    for (std::size_t index = 0; index < worker_count_; ++index) {
        workers_.emplace_back([this] {
            while (auto task = tasks_.wait_pop()) {
                (*task)();
            }
        });
    }
}

ThreadPool::~ThreadPool()
{
    shutdown();
}

void ThreadPool::submit(std::function<void()> task)
{
    if (is_shutdown()) {
        throw std::runtime_error{"cannot submit task to a shutdown ThreadPool"};
    }

    tasks_.push(std::move(task));
}

void ThreadPool::shutdown()
{
    const bool already_shutdown = shutdown_started_.exchange(true);
    if (already_shutdown) {
        return;
    }

    tasks_.close();
    workers_.clear();
}

std::size_t ThreadPool::worker_count() const noexcept
{
    return worker_count_;
}

bool ThreadPool::is_shutdown() const noexcept
{
    return shutdown_started_.load();
}

} // namespace mt
