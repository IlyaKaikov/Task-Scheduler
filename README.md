# MultiThreadScheduler

A focused modern C++20 project for demonstrating multithreading fundamentals.

The project will implement a fixed-size thread pool and a small task scheduler using standard C++ concurrency primitives. The emphasis is correctness, clear shutdown semantics, futures, synchronization, cancellation.

## Current Status

Completed:

- Milestone 1: project skeleton, CMake build, demo target, CTest wiring, and milestone documentation
- Milestone 2: thread-safe `BlockingQueue<T>` with FIFO push/pop, blocking waits, close wakeups, push rejection after close, move-only value support, and multi-producer/multi-consumer tests

Next:

- Milestone 3: fixed-size `ThreadPool` built on top of the blocking queue

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
