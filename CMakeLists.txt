cmake_minimum_required(VERSION 3.20)

project(MultiThreadScheduler
    VERSION 0.1.0
    DESCRIPTION "Modern C++ thread pool and task scheduler interview project"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(mt_scheduler
    include/blocking_queue.hpp
    src/task_scheduler.cpp
    include/task_scheduler.hpp
    src/thread_pool.cpp
    include/thread_pool.hpp
)

target_include_directories(mt_scheduler
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

if(MSVC)
    target_compile_options(mt_scheduler PRIVATE /W4 /permissive-)
else()
    target_compile_options(mt_scheduler PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(mt_demo
    examples/demo.cpp
)

target_link_libraries(mt_demo PRIVATE mt_scheduler)

enable_testing()

add_executable(thread_pool_tests
    tests/thread_pool_tests.cpp
)

target_link_libraries(thread_pool_tests PRIVATE mt_scheduler)
add_test(NAME thread_pool_tests COMMAND thread_pool_tests)

add_executable(scheduler_tests
    tests/scheduler_tests.cpp
)

target_link_libraries(scheduler_tests PRIVATE mt_scheduler)
add_test(NAME scheduler_tests COMMAND scheduler_tests)
