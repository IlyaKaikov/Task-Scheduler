#include "task_scheduler.hpp"
#include "thread_pool.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

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

bool wait_until_true_for(const std::atomic_bool& value, std::chrono::steady_clock::duration timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (value.load()) {
            return true;
        }
        std::this_thread::sleep_for(1ms);
    }

    return value.load();
}

bool wait_until_equal(const std::atomic_int& value, int expected)
{
    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (std::chrono::steady_clock::now() < deadline) {
        if (value.load() == expected) {
            return true;
        }
        std::this_thread::sleep_for(1ms);
    }

    return value.load() == expected;
}

void test_task_scheduler_api_and_shutdown_state()
{
    static_assert(!std::is_copy_constructible_v<mt::TaskScheduler>);
    static_assert(!std::is_copy_assignable_v<mt::TaskScheduler>);
    static_assert(!std::is_move_constructible_v<mt::TaskScheduler>);
    static_assert(!std::is_move_assignable_v<mt::TaskScheduler>);

    mt::ThreadPool pool{2};
    mt::TaskScheduler scheduler{pool};

    assert(!scheduler.is_shutdown());

    scheduler.schedule_after(1ms, [] {});

    scheduler.shutdown();
    assert(scheduler.is_shutdown());

    pool.shutdown();
}

void test_schedule_after_shutdown_throws()
{
    mt::ThreadPool pool{2};
    mt::TaskScheduler scheduler{pool};

    scheduler.shutdown();

    for (int attempt = 0; attempt < 3; ++attempt) {
        bool threw = false;
        try {
            scheduler.schedule_after(1ms, [] {});
        } catch (const std::runtime_error& error) {
            threw = true;
            const std::string message = error.what();
            assert(message.find("TaskScheduler") != std::string::npos);
            assert(message.find("shutdown") != std::string::npos);
        }

        assert(threw);
        assert(scheduler.is_shutdown());
    }

    pool.shutdown();
}

void test_shutdown_is_idempotent()
{
    mt::ThreadPool pool{1};
    mt::TaskScheduler scheduler{pool};

    scheduler.shutdown();
    assert(scheduler.is_shutdown());

    scheduler.shutdown();
    assert(scheduler.is_shutdown());

    pool.shutdown();
}

void test_destructor_handles_idle_coordinator()
{
    mt::ThreadPool pool{1};

    {
        mt::TaskScheduler scheduler{pool};
        assert(!scheduler.is_shutdown());
    }

    pool.shutdown();
    assert(true);
}

void test_delayed_task_eventually_runs()
{
    mt::ThreadPool pool{1};
    mt::TaskScheduler scheduler{pool};
    std::atomic_bool ran{false};

    scheduler.schedule_after(20ms, [&] {
        ran.store(true);
    });

    assert(wait_until_true(ran));

    scheduler.shutdown();
    pool.shutdown();
}

void test_delayed_task_does_not_run_too_early()
{
    mt::ThreadPool pool{1};
    mt::TaskScheduler scheduler{pool};
    std::atomic_bool ran{false};

    scheduler.schedule_after(120ms, [&] {
        ran.store(true);
    });

    std::this_thread::sleep_for(30ms);
    assert(!ran.load());
    assert(wait_until_true(ran));

    scheduler.shutdown();
    pool.shutdown();
}

void test_multiple_tasks_run_in_due_time_order_where_practical()
{
    mt::ThreadPool pool{1};
    mt::TaskScheduler scheduler{pool};
    std::atomic_int completed_count{0};
    std::mutex order_mutex;
    std::vector<int> order;

    scheduler.schedule_after(120ms, [&] {
        const std::lock_guard lock{order_mutex};
        order.push_back(1);
        completed_count.fetch_add(1);
    });

    scheduler.schedule_after(40ms, [&] {
        const std::lock_guard lock{order_mutex};
        order.push_back(2);
        completed_count.fetch_add(1);
    });

    assert(wait_until_equal(completed_count, 2));

    {
        const std::lock_guard lock{order_mutex};
        assert((order == std::vector<int>{2, 1}));
    }

    scheduler.shutdown();
    pool.shutdown();
}

void test_new_earlier_task_wakes_sleeping_coordinator()
{
    mt::ThreadPool pool{1};
    mt::TaskScheduler scheduler{pool};
    std::atomic_bool short_delay_task_ran{false};
    std::atomic_bool long_delay_task_ran{false};

    scheduler.schedule_after(800ms, [&] {
        long_delay_task_ran.store(true);
    });

    std::this_thread::sleep_for(30ms);

    scheduler.schedule_after(40ms, [&] {
        short_delay_task_ran.store(true);
    });

    assert(wait_until_true_for(short_delay_task_ran, 300ms));
    assert(!long_delay_task_ran.load());

    scheduler.shutdown();
    pool.shutdown();
}

void test_destructor_wakes_with_pending_future_task()
{
    mt::ThreadPool pool{1};
    const auto start = std::chrono::steady_clock::now();

    {
        mt::TaskScheduler scheduler{pool};
        scheduler.schedule_after(5s, [] {});
    }

    const auto elapsed = std::chrono::steady_clock::now() - start;
    assert(elapsed < 300ms);

    pool.shutdown();
}

void test_destructor_does_not_dispatch_not_due_task()
{
    mt::ThreadPool pool{1};
    std::atomic_bool ran{false};

    {
        mt::TaskScheduler scheduler{pool};
        scheduler.schedule_after(5s, [&] {
            ran.store(true);
        });
    }

    std::this_thread::sleep_for(50ms);
    assert(!ran.load());

    pool.shutdown();
}

} // namespace

int main()
{
    test_task_scheduler_api_and_shutdown_state();
    test_schedule_after_shutdown_throws();
    test_shutdown_is_idempotent();
    test_destructor_handles_idle_coordinator();
    test_delayed_task_eventually_runs();
    test_delayed_task_does_not_run_too_early();
    test_multiple_tasks_run_in_due_time_order_where_practical();
    test_new_earlier_task_wakes_sleeping_coordinator();
    test_destructor_wakes_with_pending_future_task();
    test_destructor_does_not_dispatch_not_due_task();

    return 0;
}
