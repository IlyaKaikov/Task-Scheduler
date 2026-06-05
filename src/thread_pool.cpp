#include "thread_pool.hpp"

namespace mt {

ThreadPool::ThreadPool(std::size_t worker_count)
    : worker_count_(worker_count)
{
}

std::size_t ThreadPool::worker_count() const noexcept
{
    return worker_count_;
}

} // namespace mt
