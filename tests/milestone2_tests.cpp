#include "blocking_queue.hpp"

#include <cassert>
#include <memory>
#include <type_traits>

int main()
{
    static_assert(!std::is_copy_constructible_v<mt::BlockingQueue<int>>);
    static_assert(!std::is_copy_assignable_v<mt::BlockingQueue<int>>);

    mt::BlockingQueue<int> int_queue;
    assert(!int_queue.is_closed());
    int_queue.push(42);
    assert(!int_queue.is_closed());
    assert(!int_queue.wait_pop().has_value());
    int_queue.close();
    assert(int_queue.is_closed());

    mt::BlockingQueue<std::unique_ptr<int>> move_only_queue;
    move_only_queue.push(std::make_unique<int>(7));
    assert(!move_only_queue.wait_pop().has_value());

    return 0;
}
