#include "thread_pool.hpp"

#include <iostream>

int main()
{
    mt::ThreadPool pool{4};

    std::cout << "MultiThreadScheduler demo\n";
    std::cout << "Configured workers: " << pool.worker_count() << '\n';
    std::cout << "Thread pool and scheduler behavior will be implemented in later milestones.\n";

    return 0;
}
