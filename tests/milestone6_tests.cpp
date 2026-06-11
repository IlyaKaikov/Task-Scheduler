#include "task_scheduler.hpp"
#include "thread_pool.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <thread>
#include <type_traits>

using namespace std::chrono_literals;

namespace {

bool wait_until_true(const std::atomic_bool& value)
{
    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (std::chrono::steady_clock::now() < deadline) {
        if (value.load()) {
            return true;
        }
        std::this_thread::sleep_for(1ms);
    }

    return value.load();
}

void test_scheduled_task_handle_api()
{
    static_assert(!std::is_copy_constructible_v<mt::ScheduledTaskHandle>);
    static_assert(!std::is_copy_assignable_v<mt::ScheduledTaskHandle>);
    static_assert(std::is_move_constructible_v<mt::ScheduledTaskHandle>);
    static_assert(std::is_move_assignable_v<mt::ScheduledTaskHandle>);

    mt::ScheduledTaskHandle empty_handle;
    assert(!empty_handle.is_cancelled());

    empty_handle.cancel();
    assert(!empty_handle.is_cancelled());
}

void test_schedule_after_returns_non_cancelled_handle()
{
    mt::ThreadPool pool{1};
    mt::TaskScheduler scheduler{pool};

    auto handle = scheduler.schedule_after(5s, [] {});
    assert(!handle.is_cancelled());

    scheduler.shutdown();
    pool.shutdown();
}

void test_handle_cancel_updates_handle_state()
{
    mt::ThreadPool pool{1};
    mt::TaskScheduler scheduler{pool};

    auto handle = scheduler.schedule_after(5s, [] {});
    assert(!handle.is_cancelled());

    handle.cancel();
    assert(handle.is_cancelled());

    scheduler.shutdown();
    pool.shutdown();
}

void test_handle_state_survives_move()
{
    mt::ThreadPool pool{1};
    mt::TaskScheduler scheduler{pool};

    auto handle = scheduler.schedule_after(5s, [] {});
    auto moved_handle = std::move(handle);

    assert(!moved_handle.is_cancelled());
    moved_handle.cancel();
    assert(moved_handle.is_cancelled());

    scheduler.shutdown();
    pool.shutdown();
}

void test_cancel_before_dispatch_prevents_task_running()
{
    mt::ThreadPool pool{1};
    mt::TaskScheduler scheduler{pool};
    std::atomic_bool ran{false};

    auto handle = scheduler.schedule_after(50ms, [&] {
        ran.store(true);
    });

    handle.cancel();
    assert(handle.is_cancelled());

    std::this_thread::sleep_for(200ms);
    assert(!ran.load());

    scheduler.shutdown();
    pool.shutdown();
}

void test_cancelling_one_task_does_not_cancel_other_tasks()
{
    mt::ThreadPool pool{1};
    mt::TaskScheduler scheduler{pool};
    std::atomic_bool cancelled_task_ran{false};
    std::atomic_bool active_task_ran{false};

    auto cancelled_handle = scheduler.schedule_after(50ms, [&] {
        cancelled_task_ran.store(true);
    });

    scheduler.schedule_after(60ms, [&] {
        active_task_ran.store(true);
    });

    cancelled_handle.cancel();

    assert(wait_until_true(active_task_ran));
    assert(!cancelled_task_ran.load());

    scheduler.shutdown();
    pool.shutdown();
}

void test_cancel_is_idempotent()
{
    mt::ThreadPool pool{1};
    mt::TaskScheduler scheduler{pool};
    std::atomic_bool ran{false};

    auto handle = scheduler.schedule_after(50ms, [&] {
        ran.store(true);
    });

    handle.cancel();
    handle.cancel();
    handle.cancel();

    assert(handle.is_cancelled());

    std::this_thread::sleep_for(200ms);
    assert(!ran.load());

    scheduler.shutdown();
    pool.shutdown();
}

} // namespace

int main()
{
    test_scheduled_task_handle_api();
    test_schedule_after_returns_non_cancelled_handle();
    test_handle_cancel_updates_handle_state();
    test_handle_state_survives_move();
    test_cancel_before_dispatch_prevents_task_running();
    test_cancelling_one_task_does_not_cancel_other_tasks();
    test_cancel_is_idempotent();

    return 0;
}
