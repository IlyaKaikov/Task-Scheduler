# MultiThreadScheduler

A focused modern C++20 project for demonstrating multithreading fundamentals.

The project implements a fixed-size thread pool and a small task scheduler using standard C++ concurrency primitives. The emphasis is correctness, clear shutdown semantics, synchronization, delayed execution, and cancellation.

## Current Status

Completed:

- Milestone 1: project skeleton, CMake build, demo target, CTest wiring, and milestone documentation
- Milestone 2: thread-safe `BlockingQueue<T>` with FIFO push/pop, blocking waits, close wakeups, push rejection after close, move-only value support, and multi-producer/multi-consumer tests
- Milestone 3: fixed-size `ThreadPool` with worker startup, `void` task submission, worker-thread execution, graceful shutdown, post-shutdown rejection, destructor joining, and focused milestone tests
- Milestone 4: thread pool robustness with caught task exceptions, idempotent shutdown, state queries, and `docs/design.md`
- Milestone 5: basic `TaskScheduler` with delayed `void` task scheduling, due-time ordering, coordinator wakeups, clean scheduler shutdown, and focused milestone tests
- Milestone 6: scheduled task cancellation, end-to-end demo behavior, and final documentation polish


## What This Demonstrates

- Owning and joining worker threads with `std::jthread`
- Coordinating producers and consumers with mutexes and condition variables
- Graceful shutdown that drains queued pool work
- Delayed scheduling with one coordinator thread and due-time ordering
- Best-effort cancellation for tasks that have not yet been dispatched
- Exception containment inside worker threads so one throwing task does not stop the pool

## Build

```powershell
cmake --preset vs2022-x64
cmake --build build
```

## Run

```powershell
.\build\Debug\mt_demo.exe
```

For single-configuration generators, the executable may be at:

```powershell
.\build\mt_demo.exe
```

## Test

```powershell
ctest --preset debug
```

Equivalent Visual Studio generator command:

```powershell
ctest --test-dir build -C Debug --output-on-failure
```
