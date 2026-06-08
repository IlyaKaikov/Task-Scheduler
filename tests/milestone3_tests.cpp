#include "thread_pool.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <stdexcept>
#include <thread>
#include <type_traits>

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

void test_submit_after_shutdown_throws_for_api_shape()
{
    mt::ThreadPool pool{2};
    pool.shutdown();

    bool threw = false;
    try {
        pool.submit([] {});
    } catch (const std::runtime_error&) {
        threw = true;
    }

    assert(threw);
    assert(pool.is_shutdown());
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

} // namespace

int main()
{
    test_thread_pool_api_and_worker_count();
    test_submit_after_shutdown_throws_for_api_shape();
    test_many_submitted_tasks_complete();

    return 0;
}
