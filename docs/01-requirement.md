# Requirement

## Stage 1 Name

`MiniMuduo + WebServer`

## Purpose

The first stage is not another copy of an existing WebServer. It is a full engineering pass over a muduo-inspired Reactor network library, with a small HTTP WebServer as the application layer.

## Functional Scope

Required:

- run on Linux
- use non-blocking socket fd
- use epoll as the IO multiplexing backend
- default to LT trigger mode
- support ET trigger mode through configuration after LT is stable
- support single-thread Echo Server first
- support multi-thread Reactor later
- support basic HTTP/1.1 request parsing
- support static response or static file response
- support Keep-Alive
- support connection timeout through TimerQueue
- provide build scripts and CMake build
- provide unit, integration, and manual test records
- provide benchmark records

Out of scope for Stage 1:

- HTTPS/TLS
- HTTP/2
- full HTTP framework features
- production-grade logging system
- distributed deployment
- coroutine as the main network model
- Windows support
- Boost.Asio implementation

## Non-Functional Requirements

- Code should be readable and explainable.
- Core ownership and lifetime rules must be documented.
- Network behavior should be validated by tests or manual checks.
- Each important design choice should be recorded in `docs/02-tech-selection.md`.
- Each milestone should update `docs/05-development-log.md`.

## Completion Criteria

- `csl_smoke` builds and runs.
- Echo Server milestone builds and runs.
- HTTP WebServer milestone builds and runs.
- Core modules have focused tests where practical.
- `wrk` or `ab` benchmark result is recorded.
- At least one sanitizer or Valgrind run is recorded.
- Final review is written in `docs/08-review.md`.

