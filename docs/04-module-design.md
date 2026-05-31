# Module Design

This document records target responsibilities before implementation. Details may change as the code becomes concrete.

## base

`noncopyable`

- disable copy construction and copy assignment
- used by objects with ownership or thread affinity

`Timestamp`

- represent time points
- support timer and log output

`Logger`

- provide simple log macros or stream-style logging
- keep implementation small in Stage 1

## net

`EventLoop`

- owns the event loop of one thread
- calls `Poller::poll`
- dispatches active `Channel` callbacks
- supports run-in-loop and queue-in-loop
- uses eventfd to wake up from another thread

`Channel`

- binds one fd to event callbacks
- stores interested events and returned events
- does not own fd

`Poller`

- abstract IO multiplexing interface
- supports update and remove channel

`EpollPoller`

- Linux epoll implementation of `Poller`
- owns epoll fd
- translates epoll events to active channels

`InetAddress`

- wraps IPv4 address and port

`Socket`

- wraps socket fd operations
- bind, listen, accept, shutdown write, set non-blocking, reuse address

`Acceptor`

- owns listening socket and accept channel
- accepts new connections
- invokes new connection callback

`Buffer`

- stores input and output bytes
- handles append, retrieve, readable, writable, and expansion

`TcpConnection`

- represents an established TCP connection
- owns connection state, channel, input buffer, and output buffer
- manages read, write, close, error, and shutdown behavior

`TcpServer`

- owns Acceptor and connection map
- distributes connections to EventLoopThreadPool
- exposes callbacks for connection and message handling

`EventLoopThread`

- starts one thread with one EventLoop
- returns EventLoop pointer after initialization

`EventLoopThreadPool`

- manages multiple EventLoopThread instances
- selects next loop for a new connection

## timer

`Timer`

- stores expiration time, callback, interval, and repeat flag

`TimerId`

- lightweight handle for canceling timers

`TimerQueue`

- manages timers for an EventLoop
- wakes loop when next timeout is ready
- supports one-shot and repeated timers

## http

`HttpRequest`

- stores method, path, version, headers, and body

`HttpResponse`

- stores status code, headers, body, keep-alive flag
- serializes response to Buffer

`HttpContext`

- stores parser state for one connection
- supports incomplete request parsing

`HttpServer`

- builds on `TcpServer`
- handles request callback and response writing

