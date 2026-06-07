#include "thread_pool.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <functional>
#include <memory>
#include <stdexcept>
#include <thread>
#include <type_traits>

using namespace std::chrono_literals;

namespace {

void test_thread_pool_api()
{
    static_assert(!std::is_copy_constructible_v<mt::ThreadPool>);
    static_assert(!std::is_copy_assignable_v<mt::ThreadPool>);
    static_assert(!std::is_move_constructible_v<mt::ThreadPool>);
    static_assert(!std::is_move_assignable_v<mt::ThreadPool>);

    mt::ThreadPool pool{3};
    assert(pool.worker_count() == 3);
}

void test_zero_workers_throw()
{
    bool threw = false;
    try {
        mt::ThreadPool pool{0};
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    assert(threw);
}

void test_shutdown_is_idempotent()
{
    mt::ThreadPool pool{2};

    pool.shutdown();
    pool.shutdown();

    assert(pool.worker_count() == 2);
}

void test_destructor_joins_idle_workers()
{
    {
        mt::ThreadPool pool{4};
        assert(pool.worker_count() == 4);
    }
}

void test_repeated_construct_shutdown()
{
    for (int iteration = 0; iteration < 20; ++iteration) {
        mt::ThreadPool pool{2};
        std::this_thread::sleep_for(1ms);
        pool.shutdown();
    }
}

void test_submit_returns_value()
{
    mt::ThreadPool pool{2};

    auto result = pool.submit([] {
        return 42;
    });

    assert(result.get() == 42);
}

void test_submit_forwards_callable_arguments()
{
    mt::ThreadPool pool{2};

    auto result = pool.submit(std::plus<int>{}, 2, 5);

    assert(result.get() == 7);
}

void test_submit_void_task()
{
    mt::ThreadPool pool{2};
    std::atomic_int counter{0};

    auto done = pool.submit([&] {
        counter.fetch_add(1);
    });

    done.get();
    assert(counter.load() == 1);
}

void test_submit_captured_state()
{
    mt::ThreadPool pool{2};
    int base = 10;

    auto result = pool.submit([base](int value) {
        return base + value;
    },
        5);

    assert(result.get() == 15);
}

void test_submit_move_only_argument()
{
    mt::ThreadPool pool{2};

    auto result = pool.submit(
        [](std::unique_ptr<int> value) {
            return *value;
        },
        std::make_unique<int>(9));

    assert(result.get() == 9);
}

} // namespace

int main()
{
    test_thread_pool_api();
    test_zero_workers_throw();
    test_shutdown_is_idempotent();
    test_destructor_joins_idle_workers();
    test_repeated_construct_shutdown();
    test_submit_returns_value();
    test_submit_forwards_callable_arguments();
    test_submit_void_task();
    test_submit_captured_state();
    test_submit_move_only_argument();

    return 0;
}
