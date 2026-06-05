# MultiThreadScheduler

A focused modern C++20 project for demonstrating multithreading fundamentals.

The project will implement a fixed-size thread pool and a small task scheduler using standard C++ concurrency primitives. The emphasis is correctness, clear shutdown semantics, futures, synchronization, cancellation.

## Current Status

Milestone 1 is the project skeleton:

- CMake build setup
- Library target
- Demo executable
- CTest test targets
- Milestone documentation

## Build

```powershell
cmake -S . -B build
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
ctest --test-dir build --output-on-failure
```

With Visual Studio generators, include the configuration:

```powershell
ctest --test-dir build -C Debug --output-on-failure
```