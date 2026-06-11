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
    include/task_scheduler.hpp
    include/thread_pool.hpp
    src/task_scheduler.cpp
    src/thread_pool.cpp
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
    src/demo.cpp
)

target_link_libraries(mt_demo PRIVATE mt_scheduler)

enable_testing()

add_executable(milestone2_tests
    tests/milestone2_tests.cpp
)

target_link_libraries(milestone2_tests PRIVATE mt_scheduler)
add_test(NAME milestone2_tests COMMAND milestone2_tests)

add_executable(milestone3_tests
    tests/milestone3_tests.cpp
)

target_link_libraries(milestone3_tests PRIVATE mt_scheduler)
add_test(NAME milestone3_tests COMMAND milestone3_tests)

add_executable(milestone5_tests
    tests/milestone5_tests.cpp
)

target_link_libraries(milestone5_tests PRIVATE mt_scheduler)
add_test(NAME milestone5_tests COMMAND milestone5_tests)

add_executable(milestone6_tests
    tests/milestone6_tests.cpp
)

target_link_libraries(milestone6_tests PRIVATE mt_scheduler)
add_test(NAME milestone6_tests COMMAND milestone6_tests)
