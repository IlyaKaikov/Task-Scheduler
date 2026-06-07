#pragma once

#include "blocking_queue.hpp"

#include <cstddef>
#include <future>
#include <functional>
#include <memory>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
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

    template <typename Callable, typename... Args>
    auto submit(Callable&& callable, Args&&... args)
        -> std::future<std::invoke_result_t<std::decay_t<Callable>, std::decay_t<Args>...>>;

    void shutdown() noexcept;

    [[nodiscard]] std::size_t worker_count() const noexcept;

private:
    void worker_loop();

    std::size_t worker_count_;
    BlockingQueue<std::function<void()>> tasks_;
    std::vector<std::jthread> workers_;
};

template <typename Callable, typename... Args>
auto ThreadPool::submit(Callable&& callable, Args&&... args)
    -> std::future<std::invoke_result_t<std::decay_t<Callable>, std::decay_t<Args>...>>
{
    using StoredCallable = std::decay_t<Callable>;
    using StoredArgs = std::tuple<std::decay_t<Args>...>;
    using Result = std::invoke_result_t<StoredCallable, std::decay_t<Args>...>;

    auto stored_callable = std::make_shared<StoredCallable>(std::forward<Callable>(callable));
    auto stored_args = std::make_shared<StoredArgs>(std::forward<Args>(args)...);
    auto task = std::make_shared<std::packaged_task<Result()>>(
        [stored_callable, stored_args]() mutable -> Result {
            return std::apply(
                [&](auto&... args) -> Result {
                    return std::invoke(std::move(*stored_callable), std::move(args)...);
                },
                *stored_args);
        });

    auto future = task->get_future();

    try {
        tasks_.push([task] {
            (*task)();
        });
    } catch (const std::runtime_error&) {
        throw std::runtime_error{"cannot submit task to a shut down ThreadPool"};
    }

    return future;
}

} // namespace mt
