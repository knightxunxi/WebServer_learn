# Tech Selection

## C++ Standard

Use C++20.

Reasons:

- modern standard library features are useful for new code
- `std::jthread`, `std::stop_token`, `std::span`, and improved chrono are valuable
- C++20 is acceptable for a learning and GitHub demonstration repository

Coroutine decision:

- do not use coroutine as the Stage 1 main path
- keep callback-style Reactor as the core model
- add coroutine API later as an experiment after the lifecycle model is clear

## Platform

Stage 1 is Linux only.

Reasons:

- Linux is the primary environment for C++ server-side infrastructure
- epoll, fd lifecycle, non-blocking IO, and event-driven design should be learned directly
- cross-platform abstraction can be evaluated later with Boost.Asio

## IO Backend

Use epoll.

Default mode:

- LT with non-blocking fd

Reserved extension:

- ET configurable mode

Rationale:

- LT is easier to validate in the first stable implementation
- ET is useful for deeper study, but requires strict read/write loops until `EAGAIN`
- supporting both modes later enables benchmark and behavior comparison

## Concurrency Model

Use one loop per thread.

Planned model:

- main loop accepts new connections
- sub loops handle established connection IO
- each `EventLoop` belongs to one thread
- cross-thread task submission uses pending functor queue plus eventfd wakeup

## Third-Party Dependencies

Stage 1 keeps dependencies minimal.

Allowed:

- CMake
- system Linux APIs
- optional GoogleTest later if manual tests become hard to maintain

Avoid in Stage 1:

- Boost.Asio
- spdlog
- fmt
- full-featured HTTP parser library

## Testing Tools

Planned:

- CTest for baseline test execution
- curl, nc, telnet for manual checks
- wrk or ab for benchmark
- ASan, UBSan, Valgrind for memory and undefined behavior checks
- gdb, strace, lsof, perf for debugging and profiling

