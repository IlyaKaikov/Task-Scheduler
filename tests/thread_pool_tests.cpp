#include "thread_pool.hpp"

#include <cassert>

int main()
{
    const mt::ThreadPool pool{2};
    assert(pool.worker_count() == 2);

    return 0;
}
