# C++ Server Lab

`cpp-server-lab` is a learning-oriented C++ server-side repository. The first stage focuses on a muduo-inspired Reactor network library and an HTTP WebServer, with documentation, tests, benchmarks, and review notes maintained throughout development.

## Goals

- Build a Linux C++ server foundation from requirements to review.
- Understand Reactor, epoll, non-blocking IO, connection lifetime, timers, and one-loop-per-thread.
- Keep decisions, issues, test plans, and benchmark results in versioned documents.
- Prepare the project so it can be cloned and built on a Linux virtual machine.

## Current Stage

Stage 1: `MiniMuduo + WebServer`

- Language: C++20
- Platform: Linux first
- IO: epoll with non-blocking fd
- Trigger mode: LT by default, ET reserved as a configurable extension
- Concurrency model: one loop per thread
- Core style: callback-style Reactor
- Coroutine: reserved for later experiments, not the first-stage main path

## Repository Layout

```text
.
├── apps/                         # Runnable examples and services
│   ├── echo_server/
│   ├── smoke/
│   └── web_server/
├── benchmarks/                   # Benchmark scripts and results
├── docs/                         # Requirements, design, logs, testing, review
├── experiments/                  # Optional experiments such as coroutine API
├── include/csl/                  # Public headers
│   ├── base/
│   ├── http/
│   ├── net/
│   └── timer/
├── scripts/                      # Build and test helpers
├── src/                          # Implementations
└── tests/                        # Unit and integration tests
```

## Linux Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCSL_BUILD_TESTS=ON
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure
```

Or use the helper scripts:

```bash
bash scripts/build.sh
bash scripts/test.sh
```

## Documentation

- [Roadmap](docs/00-roadmap.md)
- [Requirement](docs/01-requirement.md)
- [Tech Selection](docs/02-tech-selection.md)
- [Architecture](docs/03-architecture.md)
- [Module Design](docs/04-module-design.md)
- [Development Log](docs/05-development-log.md)
- [Problems](docs/06-problems.md)
- [Benchmark](docs/07-benchmark.md)
- [Review](docs/08-review.md)
- [Testing](docs/09-testing.md)
- [GitHub and Linux Workflow](docs/10-github-linux-workflow.md)

## Development Workflow

1. Update requirement or design documents before implementing a meaningful module.
2. Implement the smallest runnable milestone.
3. Add tests for deterministic logic and integration checks for network behavior.
4. Record bugs, tradeoffs, and debug notes in `docs/06-problems.md`.
5. Run build, tests, and benchmark before finishing a milestone.
6. Write a review note after each stage.

