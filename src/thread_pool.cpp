#include "thread_pool.hpp"

#include <stdexcept>

namespace mt {

ThreadPool::ThreadPool(std::size_t worker_count)
    : worker_count_(worker_count)
{
    if (worker_count_ == 0) {
        throw std::invalid_argument{"ThreadPool worker count must be greater than zero"};
    }

    workers_.reserve(worker_count_);
    for (std::size_t index = 0; index < worker_count_; ++index) {
        workers_.emplace_back([this] {
            worker_loop();
        });
    }
}

ThreadPool::~ThreadPool()
{
    shutdown();
}

void ThreadPool::shutdown() noexcept
{
    tasks_.close();

    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

std::size_t ThreadPool::worker_count() const noexcept
{
    return worker_count_;
}

void ThreadPool::worker_loop()
{
    while (auto task = tasks_.wait_pop()) {
        (*task)();
    }
}

} // namespace mt
