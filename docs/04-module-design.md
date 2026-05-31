# 模块设计

本文记录实现前的目标职责。具体细节会随着代码推进持续修正。

## base

`noncopyable`

- 禁止拷贝构造和拷贝赋值。
- 用于具备所有权、线程归属或资源管理语义的对象。

`Timestamp`

- 表示时间点。
- 用于定时器和日志输出。

`Logger`

- 提供简单日志宏或流式日志接口。
- 第一阶段保持小实现，不追求生产级能力。

## net

`EventLoop`

- 拥有一个线程内的事件循环。
- 调用 `Poller::poll`。
- 分发活跃 `Channel` 的回调。
- 支持 run-in-loop 和 queue-in-loop。
- 通过 eventfd 支持跨线程唤醒。

`Channel`

- 将一个 fd 与事件回调绑定。
- 保存感兴趣事件和实际返回事件。
- 不拥有 fd。

`Poller`

- IO 多路复用抽象接口。
- 支持更新和移除 Channel。

`EpollPoller`

- `Poller` 的 Linux epoll 实现。
- 拥有 epoll fd。
- 将 epoll 事件转换为活跃 Channel 列表。

`InetAddress`

- 封装 IPv4 地址和端口。

`Socket`

- 封装 socket fd 操作。
- 提供 bind、listen、accept、shutdown write、set non-blocking、reuse address 等能力。

`Acceptor`

- 拥有监听 socket 和 accept channel。
- 接收新连接。
- 触发 new connection callback。

`Buffer`

- 保存输入和输出字节。
- 支持 append、retrieve、readable、writable 和自动扩容。

`TcpConnection`

- 表示一个已建立 TCP 连接。
- 管理连接状态、Channel、input buffer 和 output buffer。
- 处理读、写、关闭、错误和半关闭。

`TcpServer`

- 拥有 Acceptor 和连接表。
- 将连接分发给 EventLoopThreadPool。
- 对外暴露连接回调和消息回调。

`EventLoopThread`

- 启动一个线程和一个 EventLoop。
- 在线程初始化完成后返回 EventLoop 指针。

`EventLoopThreadPool`

- 管理多个 EventLoopThread。
- 为新连接选择下一个 EventLoop。

## timer

`Timer`

- 保存过期时间、回调、间隔和是否重复。

`TimerId`

- 作为取消定时器的轻量句柄。

`TimerQueue`

- 管理某个 EventLoop 内的定时器。
- 在最近超时时间到达时唤醒 loop。
- 支持一次性定时器和重复定时器。

## http

`HttpRequest`

- 保存 method、path、version、headers 和 body。

`HttpResponse`

- 保存状态码、headers、body 和 keep-alive 标记。
- 将响应序列化到 Buffer。

`HttpContext`

- 保存单个连接上的 HTTP 解析状态。
- 支持不完整请求解析。

`HttpServer`

- 构建在 `TcpServer` 之上。
- 处理请求回调和响应写回。

