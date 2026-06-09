#pragma once

#include "thread_pool.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace mt {

class TaskScheduler {
public:
    explicit TaskScheduler(ThreadPool& pool);
    ~TaskScheduler();

    TaskScheduler(const TaskScheduler&) = delete;
    TaskScheduler& operator=(const TaskScheduler&) = delete;
    TaskScheduler(TaskScheduler&&) = delete;
    TaskScheduler& operator=(TaskScheduler&&) = delete;

    void schedule_after(std::chrono::steady_clock::duration delay, std::function<void()> task);
    void shutdown();

    [[nodiscard]] bool is_shutdown() const noexcept;

private:
    struct ScheduledTask {
        std::chrono::steady_clock::time_point due_time;
        std::size_t sequence;
        std::function<void()> task;
    };

    struct ScheduledTaskCompare {
        [[nodiscard]] bool operator()(const ScheduledTask& left, const ScheduledTask& right) const noexcept;
    };

    void coordinator_loop(std::stop_token stop_token);

    ThreadPool& pool_;
    std::jthread coordinator_;
    std::atomic_bool shutdown_started_{false};
    std::mutex mutex_;
    std::condition_variable_any condition_;
    std::priority_queue<ScheduledTask, std::vector<ScheduledTask>, ScheduledTaskCompare> scheduled_tasks_;
    std::size_t next_sequence_{0};
};

} // namespace mt
