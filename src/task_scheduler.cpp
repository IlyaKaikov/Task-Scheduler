#include "task_scheduler.hpp"

#include <chrono>
#include <stdexcept>
#include <utility>

namespace mt {

namespace detail {
struct ScheduledTaskState {
    std::atomic_bool cancelled{false};
    std::atomic_bool dispatched{false};
};
} // namespace detail

ScheduledTaskHandle::ScheduledTaskHandle(std::shared_ptr<detail::ScheduledTaskState> state)
    : state_(std::move(state))
{
}

void ScheduledTaskHandle::cancel() noexcept
{
    if (state_) {
        state_->cancelled.store(true);
    }
}

bool ScheduledTaskHandle::is_cancelled() const noexcept
{
    return state_ && state_->cancelled.load();
}

TaskScheduler::TaskScheduler(ThreadPool& pool)
    : pool_(pool)
    , coordinator_([this](std::stop_token stop_token) {
        coordinator_loop(stop_token);
    })
{
}

TaskScheduler::~TaskScheduler()
{
    shutdown();
}

ScheduledTaskHandle TaskScheduler::schedule_after(std::chrono::steady_clock::duration delay, std::function<void()> task)
{
    auto state = std::make_shared<detail::ScheduledTaskState>();

    {
        const std::lock_guard lock{mutex_};
        if (is_shutdown()) {
            throw std::runtime_error{"cannot schedule task on a shutdown TaskScheduler"};
        }

        scheduled_tasks_.push(ScheduledTask{
            std::chrono::steady_clock::now() + delay,
            next_sequence_++,
            state,
            std::move(task),
        });
    }

    condition_.notify_one();
    return ScheduledTaskHandle{std::move(state)};
}

void TaskScheduler::shutdown()
{
    const bool already_shutdown = shutdown_started_.exchange(true);
    if (already_shutdown) {
        return;
    }

    {
        const std::lock_guard lock{mutex_};
        scheduled_tasks_ = {};
    }

    coordinator_.request_stop();
    condition_.notify_all();
    coordinator_.join();
}

bool TaskScheduler::is_shutdown() const noexcept
{
    return shutdown_started_.load();
}

bool TaskScheduler::ScheduledTaskCompare::operator()(
    const ScheduledTask& left,
    const ScheduledTask& right) const noexcept
{
    if (left.due_time == right.due_time) {
        return left.sequence > right.sequence;
    }

    return left.due_time > right.due_time;
}

void TaskScheduler::coordinator_loop(std::stop_token stop_token)
{
    std::unique_lock lock{mutex_};

    while (!stop_token.stop_requested() && !shutdown_started_.load()) {
        if (scheduled_tasks_.empty()) {
            condition_.wait(lock, stop_token, [this] {
                return shutdown_started_.load() || !scheduled_tasks_.empty();
            });
            continue;
        }

        const auto next_due_time = scheduled_tasks_.top().due_time;
        const bool should_dispatch = condition_.wait_until(lock, stop_token, next_due_time, [this, next_due_time] {
            return shutdown_started_.load()
                || scheduled_tasks_.empty()
                || scheduled_tasks_.top().due_time < next_due_time;
        });

        if (should_dispatch) {
            continue;
        }

        auto now = std::chrono::steady_clock::now();
        while (!stop_token.stop_requested()
            && !shutdown_started_.load()
            && !scheduled_tasks_.empty()
            && scheduled_tasks_.top().due_time <= now) {
            auto scheduled_task = scheduled_tasks_.top();
            scheduled_tasks_.pop();

            if (scheduled_task.state->cancelled.load()) {
                continue;
            }

            lock.unlock();
            try {
                pool_.submit(std::move(scheduled_task.task));
            } catch (...) {
            }
            lock.lock();

            now = std::chrono::steady_clock::now();
        }
    }
}

} // namespace mt
