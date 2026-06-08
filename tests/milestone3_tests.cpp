#include "thread_pool.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

using namespace std::chrono_literals;

namespace {

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

void test_thread_pool_api_and_worker_count()
{
    static_assert(!std::is_copy_constructible_v<mt::ThreadPool>);
    static_assert(!std::is_copy_assignable_v<mt::ThreadPool>);
    static_assert(!std::is_move_constructible_v<mt::ThreadPool>);
    static_assert(!std::is_move_assignable_v<mt::ThreadPool>);

    mt::ThreadPool pool{3};
    assert(pool.worker_count() == 3);
    assert(!pool.is_shutdown());

    pool.submit([] {});

    pool.shutdown();
    assert(pool.is_shutdown());

    pool.shutdown();
    assert(pool.is_shutdown());
}

void test_submit_after_shutdown_throws()
{
    mt::ThreadPool pool{2};
    pool.shutdown();

    for (int attempt = 0; attempt < 3; ++attempt) {
        bool threw = false;
        try {
            pool.submit([] {});
        } catch (const std::runtime_error& error) {
            threw = true;
            const std::string message = error.what();
            assert(message.find("ThreadPool") != std::string::npos);
            assert(message.find("shutdown") != std::string::npos);
        }

        assert(threw);
        assert(pool.is_shutdown());
    }
}

void test_many_submitted_tasks_complete()
{
    constexpr int task_count = 200;

    mt::ThreadPool pool{4};
    std::atomic_int completed_count{0};

    for (int index = 0; index < task_count; ++index) {
        pool.submit([&] {
            completed_count.fetch_add(1);
        });
    }

    assert(wait_until_equal(completed_count, task_count));

    pool.shutdown();
    assert(completed_count.load() == task_count);
    assert(pool.is_shutdown());
}

void test_tasks_run_on_worker_threads()
{
    constexpr int task_count = 20;

    mt::ThreadPool pool{3};
    const auto caller_thread_id = std::this_thread::get_id();
    std::atomic_int completed_count{0};
    std::mutex thread_ids_mutex;
    std::vector<std::thread::id> thread_ids;

    thread_ids.reserve(task_count);
    for (int index = 0; index < task_count; ++index) {
        pool.submit([&] {
            {
                const std::lock_guard lock{thread_ids_mutex};
                thread_ids.push_back(std::this_thread::get_id());
            }
            completed_count.fetch_add(1);
        });
    }

    assert(wait_until_equal(completed_count, task_count));

    bool saw_worker_thread = false;
    {
        const std::lock_guard lock{thread_ids_mutex};
        assert(thread_ids.size() == task_count);
        for (const auto thread_id : thread_ids) {
            if (thread_id != caller_thread_id) {
                saw_worker_thread = true;
            }
        }
    }

    pool.shutdown();
    assert(saw_worker_thread);
}

void test_shutdown_drains_queued_work_before_returning()
{
    constexpr int worker_count = 2;
    constexpr int task_count = 6;

    mt::ThreadPool pool{worker_count};
    std::atomic_int started_count{0};
    std::atomic_int completed_count{0};
    std::atomic_bool shutdown_returned{false};
    std::mutex gate_mutex;
    std::condition_variable gate_condition;
    bool gate_open = false;

    for (int index = 0; index < task_count; ++index) {
        pool.submit([&] {
            started_count.fetch_add(1);

            std::unique_lock lock{gate_mutex};
            gate_condition.wait(lock, [&] {
                return gate_open;
            });
            lock.unlock();

            completed_count.fetch_add(1);
        });
    }

    assert(wait_until_equal(started_count, worker_count));
    assert(completed_count.load() == 0);

    std::jthread shutdown_thread{[&] {
        pool.shutdown();
        shutdown_returned.store(true);
    }};

    std::this_thread::sleep_for(50ms);
    assert(!shutdown_returned.load());
    assert(completed_count.load() == 0);

    {
        const std::lock_guard lock{gate_mutex};
        gate_open = true;
    }
    gate_condition.notify_all();

    shutdown_thread.join();

    assert(shutdown_returned.load());
    assert(completed_count.load() == task_count);
    assert(pool.is_shutdown());
}

void test_destructor_handles_idle_workers()
{
    {
        mt::ThreadPool pool{3};
        assert(!pool.is_shutdown());
    }

    assert(true);
}

void test_destructor_drains_work_and_joins()
{
    constexpr int task_count = 50;

    std::atomic_int completed_count{0};

    {
        mt::ThreadPool pool{4};
        for (int index = 0; index < task_count; ++index) {
            pool.submit([&] {
                completed_count.fetch_add(1);
            });
        }
    }

    assert(completed_count.load() == task_count);
}

} // namespace

int main()
{
    test_thread_pool_api_and_worker_count();
    test_submit_after_shutdown_throws();
    test_many_submitted_tasks_complete();
    test_tasks_run_on_worker_threads();
    test_shutdown_drains_queued_work_before_returning();
    test_destructor_handles_idle_workers();
    test_destructor_drains_work_and_joins();

    return 0;
}
