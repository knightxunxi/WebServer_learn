# Architecture

Stage 1 follows a muduo-inspired Reactor architecture while keeping implementation scope small enough for learning.

## Layering

```text
apps
  web_server / echo_server
http
  HttpServer / HttpContext / HttpRequest / HttpResponse
net
  TcpServer / TcpConnection / Acceptor / EventLoop / Channel / Poller
timer
  TimerQueue / Timer / TimerId
base
  Logger / Timestamp / noncopyable
```

## Main Runtime Model

```text
main thread
  EventLoop
    Acceptor
      accept new connection
      assign connection to sub EventLoop

worker thread N
  EventLoop
    TcpConnection
      read
      decode
      callback
      write
      close
```

## Important Ownership Rules

- `EventLoop` belongs to exactly one thread.
- `Channel` does not own fd. It only stores fd event interests and callbacks.
- `Poller` owns the epoll instance and active event collection.
- `TcpConnection` owns the connection lifecycle abstraction.
- `Socket` wraps fd operations and closes fd through RAII where appropriate.
- `Buffer` owns read/write memory and hides partial read/write details.
- `TimerQueue` is reusable by HTTP timeout, WebSocket heartbeat, KV TTL, and MQ delay tasks.

## Connection Lifecycle

Initial target lifecycle:

```text
accept
  -> set non-blocking
  -> create TcpConnection
  -> register read event
  -> read data into Buffer
  -> call message callback
  -> write response or queue output
  -> handle peer close / error / timeout
  -> remove Channel
  -> destroy TcpConnection
```

## Trigger Mode Strategy

The architecture should allow:

```text
TriggerMode::Level
TriggerMode::Edge
```

LT is implemented and stabilized first. ET is added after read/write behavior, error handling, and tests are stable.

## Coroutine Position

Coroutine is not part of the first-stage core architecture. A later experiment may wrap callback-based async operations with `co_await`, but it should not disturb the Reactor baseline.

