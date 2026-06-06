#include "blocking_queue.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <vector>

using namespace std::chrono_literals;

namespace {

void test_blocking_queue_api()
{
    static_assert(!std::is_copy_constructible_v<mt::BlockingQueue<int>>);
    static_assert(!std::is_copy_assignable_v<mt::BlockingQueue<int>>);

    mt::BlockingQueue<int> int_queue;
    assert(!int_queue.is_closed());
    int_queue.push(42);
    assert(!int_queue.is_closed());
    assert(int_queue.wait_pop() == 42);
    int_queue.close();
    assert(int_queue.is_closed());
    assert(!int_queue.wait_pop().has_value());
}

void test_fifo_ordering()
{
    mt::BlockingQueue<int> fifo_queue;
    fifo_queue.push(1);
    fifo_queue.push(2);
    fifo_queue.push(3);
    assert(fifo_queue.wait_pop() == 1);
    assert(fifo_queue.wait_pop() == 2);
    assert(fifo_queue.wait_pop() == 3);
    fifo_queue.close();
    assert(!fifo_queue.wait_pop().has_value());
}

void test_batch_drain()
{
    mt::BlockingQueue<int> batch_queue;
    for (int value = 0; value < 5; ++value) {
        batch_queue.push(value);
    }
    batch_queue.close();

    std::vector<int> drained;
    while (auto value = batch_queue.wait_pop()) {
        drained.push_back(*value);
    }

    assert((drained == std::vector<int>{0, 1, 2, 3, 4}));
}

void test_move_only_values()
{
    mt::BlockingQueue<std::unique_ptr<int>> move_only_queue;
    move_only_queue.push(std::make_unique<int>(7));
    auto moved_value = move_only_queue.wait_pop();
    assert(moved_value.has_value());
    assert(**moved_value == 7);
    move_only_queue.close();
    assert(!move_only_queue.wait_pop().has_value());
}

void test_wait_pop_blocks_until_push()
{
    mt::BlockingQueue<int> queue;
    std::atomic_bool waiting{false};
    std::atomic_bool completed{false};
    int popped_value = 0;

    std::jthread worker{[&] {
        waiting.store(true);
        auto value = queue.wait_pop();
        assert(value.has_value());
        popped_value = *value;
        completed.store(true);
    }};

    while (!waiting.load()) {
        std::this_thread::yield();
    }

    std::this_thread::sleep_for(50ms);
    assert(!completed.load());

    queue.push(99);
    worker.join();

    assert(completed.load());
    assert(popped_value == 99);
}

void test_close_preserves_queued_items()
{
    mt::BlockingQueue<int> queue;
    queue.push(10);
    queue.push(20);
    queue.close();

    assert(queue.is_closed());
    assert(queue.wait_pop() == 10);
    assert(queue.wait_pop() == 20);
    assert(!queue.wait_pop().has_value());
}

void test_close_wakes_all_waiters()
{
    constexpr int worker_count = 3;

    mt::BlockingQueue<int> queue;
    std::atomic_int waiting_count{0};
    std::atomic_int completed_count{0};
    std::atomic_int nullopt_count{0};
    std::vector<std::jthread> workers;

    workers.reserve(worker_count);
    for (int index = 0; index < worker_count; ++index) {
        workers.emplace_back([&] {
            waiting_count.fetch_add(1);
            auto value = queue.wait_pop();
            if (!value.has_value()) {
                nullopt_count.fetch_add(1);
            }
            completed_count.fetch_add(1);
        });
    }

    while (waiting_count.load() != worker_count) {
        std::this_thread::yield();
    }

    std::this_thread::sleep_for(50ms);
    assert(completed_count.load() == 0);

    queue.close();

    for (auto& worker : workers) {
        worker.join();
    }

    assert(completed_count.load() == worker_count);
    assert(nullopt_count.load() == worker_count);
}

void test_close_is_idempotent()
{
    mt::BlockingQueue<int> queue;
    queue.close();
    queue.close();

    assert(queue.is_closed());
    assert(!queue.wait_pop().has_value());
}

void test_push_after_close_throws()
{
    mt::BlockingQueue<int> queue;
    queue.close();

    bool threw = false;
    try {
        queue.push(1);
    } catch (const std::runtime_error&) {
        threw = true;
    }

    assert(threw);
    assert(!queue.wait_pop().has_value());
}

void test_push_after_draining_closed_queue_throws()
{
    mt::BlockingQueue<int> queue;
    queue.push(1);
    queue.close();

    assert(queue.wait_pop() == 1);
    assert(!queue.wait_pop().has_value());

    bool threw = false;
    try {
        queue.push(2);
    } catch (const std::runtime_error&) {
        threw = true;
    }

    assert(threw);
    assert(!queue.wait_pop().has_value());
}

void test_multi_producer_multi_consumer()
{
    constexpr int producer_count = 4;
    constexpr int consumer_count = 3;
    constexpr int values_per_producer = 100;
    constexpr int total_values = producer_count * values_per_producer;

    mt::BlockingQueue<int> queue;
    std::vector<int> seen_counts(total_values, 0);
    std::mutex seen_mutex;
    std::atomic_int consumed_count{0};
    std::vector<std::jthread> consumers;

    consumers.reserve(consumer_count);
    for (int index = 0; index < consumer_count; ++index) {
        consumers.emplace_back([&] {
            while (auto value = queue.wait_pop()) {
                assert(*value >= 0);
                assert(*value < total_values);

                {
                    const std::lock_guard lock{seen_mutex};
                    ++seen_counts[*value];
                }
                consumed_count.fetch_add(1);
            }
        });
    }

    std::vector<std::jthread> producers;
    producers.reserve(producer_count);
    for (int producer = 0; producer < producer_count; ++producer) {
        producers.emplace_back([&, producer] {
            const int start = producer * values_per_producer;
            const int end = start + values_per_producer;
            for (int value = start; value < end; ++value) {
                queue.push(value);
            }
        });
    }

    for (auto& producer : producers) {
        producer.join();
    }

    queue.close();

    for (auto& consumer : consumers) {
        consumer.join();
    }

    assert(consumed_count.load() == total_values);
    for (int count : seen_counts) {
        assert(count == 1);
    }
}

} // namespace

int main()
{
    test_blocking_queue_api();
    test_fifo_ordering();
    test_batch_drain();
    test_move_only_values();
    test_wait_pop_blocks_until_push();
    test_close_preserves_queued_items();
    test_close_wakes_all_waiters();
    test_close_is_idempotent();
    test_push_after_close_throws();
    test_push_after_draining_closed_queue_throws();
    test_multi_producer_multi_consumer();

    return 0;
}
