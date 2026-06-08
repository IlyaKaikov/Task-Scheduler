# MultiThreadScheduler

A focused modern C++20 project for demonstrating multithreading fundamentals.

The project will implement a fixed-size thread pool and a small task scheduler using standard C++ concurrency primitives. The emphasis is correctness, clear shutdown semantics, synchronization, delayed execution, and cancellation.

## Current Status

Completed:

- Milestone 1: project skeleton, CMake build, demo target, CTest wiring, and milestone documentation
- Milestone 2: thread-safe `BlockingQueue<T>` with FIFO push/pop, blocking waits, close wakeups, push rejection after close, move-only value support, and multi-producer/multi-consumer tests
- Milestone 3: fixed-size `ThreadPool` with worker startup, `void` task submission, worker-thread execution, graceful shutdown, post-shutdown rejection, destructor joining, and focused milestone tests
- Milestone 4: thread pool robustness with caught task exceptions, idempotent shutdown, state queries, and `docs/design.md`

Next:

- Milestone 5: basic delayed task scheduler

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
