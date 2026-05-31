# Roadmap

This repository is developed as a long-term C++ server-side learning project. Each stage should include requirements, design, implementation, tests, benchmark data, and review notes.

## Stage 1: MiniMuduo + WebServer

Goal: implement a muduo-inspired Reactor network library and build an HTTP WebServer on top of it.

Main topics:

- Linux non-blocking network programming
- epoll LT first, ET as a configurable extension
- Reactor and one-loop-per-thread
- Channel, Poller, EventLoop, Acceptor, TcpConnection, TcpServer
- Buffer and connection lifetime
- TimerQueue and timeout handling
- HTTP request parsing and static response
- CMake, tests, benchmark, sanitizer, and review

Milestones:

1. Project baseline: CMake, docs, scripts, Git.
2. Base utilities: noncopyable, Timestamp, Logger.
3. Event loop baseline: EventLoop, Channel, Poller, EpollPoller.
4. TCP baseline: Socket, InetAddress, Acceptor, TcpConnection.
5. Echo server: single-thread runnable server.
6. Buffer and write path: handle partial read/write.
7. Multi-thread Reactor: EventLoopThread and EventLoopThreadPool.
8. TimerQueue: timeout and scheduled task support.
9. HTTP server: parse request and send response.
10. Testing, benchmark, review, and README polish.

## Stage 2: WebSocket Long-Connection Server

Goal: build long-connection service capability on top of the network layer.

Topics:

- WebSocket handshake and frame codec
- heartbeat and timeout
- rooms and broadcast
- single chat and group chat
- reconnect and ACK
- basic authentication

## Stage 3: KV Store

Goal: build a simple Redis-like in-memory KV service.

Topics:

- TCP protocol design
- GET, SET, DEL, EXPIRE
- TTL and TimerQueue reuse
- persistence snapshot or append-only log
- simple LRU
- slow query log

## Stage 4: Message Queue

Goal: build a simple topic-based message queue.

Topics:

- producer and consumer
- ACK and retry
- dead letter queue
- sequential consumption
- persistence log
- consumer group basics

## Stage 5: RPC Framework

Goal: build service-to-service communication capability.

Topics:

- codec and request id
- timeout and error code
- sync and async call
- service registry
- simple load balancing

## Later Experiments

- C++20 coroutine-based async API
- ET and LT benchmark comparison
- lock-free queue evaluation
- Boost.Asio cross-platform prototype
- Seastar reading notes

