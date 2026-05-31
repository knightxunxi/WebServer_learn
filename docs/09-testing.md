# Testing

Testing is a required part of Stage 1 because the project is intended to follow an engineering workflow instead of only producing code.

## Test Levels

Unit tests:

- Buffer
- HTTP parser
- TimerQueue
- configuration parsing if added

Integration tests:

- Echo Server single connection
- Echo Server multiple connections
- HTTP request and response
- Keep-Alive
- client abnormal disconnect
- server active close

Manual tests:

- `curl`
- `nc`
- `telnet`
- browser request for static response

Benchmark tests:

- `wrk`
- `ab`

Debug and quality checks:

- ASan
- UBSan
- Valgrind
- gdb
- strace
- lsof
- perf

## Current Baseline

The initial repository has one smoke test through CTest:

```bash
cmake -S . -B build -DCSL_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## First Testing Tasks

1. Add Buffer unit tests after Buffer is implemented.
2. Add EventLoop smoke integration test after EventLoop and Channel are implemented.
3. Add Echo Server manual test notes after the first runnable server.
4. Add HTTP parser unit tests before exposing HTTP Server.

