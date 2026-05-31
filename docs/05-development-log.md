# Development Log

## 2026-05-31

Initial repository planning and scaffold.

Decisions:

- Repository is organized as a long-term C++ server-side learning project.
- Stage 1 is `MiniMuduo + WebServer`.
- Use C++20.
- Use Linux epoll as the first backend.
- Default trigger mode is LT.
- ET is reserved as a configurable extension.
- Callback-style Reactor is the main Stage 1 model.
- Coroutine is reserved for later experiments.
- Documentation will be updated during development instead of written only at the end.

Created baseline:

- README
- docs for roadmap, requirement, tech selection, architecture, module design, testing, workflow
- CMake baseline
- smoke app and smoke test
- scripts for build and test

