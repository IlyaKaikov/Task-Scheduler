#include "blocking_queue.hpp"

#include <cassert>
#include <memory>
#include <type_traits>
#include <vector>

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
    assert(!int_queue.wait_pop().has_value());
    int_queue.close();
    assert(int_queue.is_closed());
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
    assert(!fifo_queue.wait_pop().has_value());
}

void test_batch_drain()
{
    mt::BlockingQueue<int> batch_queue;
    for (int value = 0; value < 5; ++value) {
        batch_queue.push(value);
    }

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
    assert(!move_only_queue.wait_pop().has_value());
}

} // namespace

int main()
{
    test_blocking_queue_api();
    test_fifo_ordering();
    test_batch_drain();
    test_move_only_values();

    return 0;
}
