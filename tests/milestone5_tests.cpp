#include "task_scheduler.hpp"
#include "thread_pool.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <stdexcept>
#include <string>
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

} // namespace

int main()
{
    test_task_scheduler_api_and_shutdown_state();
    test_schedule_after_shutdown_throws();
    test_shutdown_is_idempotent();
    test_destructor_handles_idle_coordinator();
    test_delayed_task_eventually_runs();
    test_delayed_task_does_not_run_too_early();

    return 0;
}
