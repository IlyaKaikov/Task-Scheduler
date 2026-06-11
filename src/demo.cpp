#include "task_scheduler.hpp"
#include "thread_pool.hpp"

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
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
        std::this_thread::sleep_for(5ms);
    }

    return value.load() == expected;
}

} // namespace

int main()
{
    mt::ThreadPool pool{4};
    mt::TaskScheduler scheduler{pool};

    std::atomic_int pool_tasks_completed{0};
    std::atomic_int delayed_tasks_completed{0};
    std::atomic_bool cancelled_task_ran{false};
    std::mutex events_mutex;
    std::vector<std::string> events;

    std::cout << "MultiThreadScheduler demo\n";
    std::cout << "Configured workers: " << pool.worker_count() << '\n';

    for (int index = 0; index < 8; ++index) {
        pool.submit([&pool_tasks_completed, index] {
            int result = 0;
            for (int value = 0; value < 1000 + index; ++value) {
                result += value % 7;
            }
            (void)result;
            pool_tasks_completed.fetch_add(1);
        });
    }

    pool.submit([] {
        throw std::runtime_error{"intentional demo exception"};
    });

    pool.submit([&pool_tasks_completed] {
        pool_tasks_completed.fetch_add(1);
    });

    scheduler.schedule_after(75ms, [&] {
        {
            const std::lock_guard lock{events_mutex};
            events.push_back("first delayed task");
        }
        delayed_tasks_completed.fetch_add(1);
    });

    scheduler.schedule_after(125ms, [&] {
        {
            const std::lock_guard lock{events_mutex};
            events.push_back("second delayed task");
        }
        delayed_tasks_completed.fetch_add(1);
    });

    auto cancelled = scheduler.schedule_after(150ms, [&] {
        cancelled_task_ran.store(true);
    });
    cancelled.cancel();

    const bool pool_work_finished = wait_until_equal(pool_tasks_completed, 9);
    const bool delayed_work_finished = wait_until_equal(delayed_tasks_completed, 2);

    std::this_thread::sleep_for(100ms);

    scheduler.shutdown();
    pool.shutdown();

    std::cout << "Pool tasks completed: " << pool_tasks_completed.load() << " / 9\n";
    std::cout << "Pool survived throwing task: " << (pool_work_finished ? "yes" : "no") << '\n';
    std::cout << "Delayed tasks completed: " << delayed_tasks_completed.load() << " / 2\n";
    std::cout << "Cancelled task ran: " << (cancelled_task_ran.load() ? "yes" : "no") << '\n';
    std::cout << "Cancelled handle state: " << (cancelled.is_cancelled() ? "cancelled" : "active") << '\n';

    {
        const std::lock_guard lock{events_mutex};
        std::cout << "Scheduled events:";
        for (const auto& event : events) {
            std::cout << ' ' << '[' << event << ']';
        }
        std::cout << '\n';
    }

    std::cout << "Clean shutdown: " << ((scheduler.is_shutdown() && pool.is_shutdown()) ? "yes" : "no") << '\n';

    if (!pool_work_finished || !delayed_work_finished || cancelled_task_ran.load()) {
        return 1;
    }

    return 0;
}
