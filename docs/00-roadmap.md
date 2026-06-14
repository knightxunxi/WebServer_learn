# 学习路线

本仓库作为长期 C++ 服务端学习项目维护。每个阶段都应包含需求分析、架构设计、代码实现、测试验证、压测记录和阶段复盘。

## 第一阶段：Boost.Asio + WebServer

目标：基于 Boost.Asio 实现跨平台异步 TCP/HTTP 服务，并完成需求分析、架构设计、测试验证和阶段复盘。

历史说明：此前已完成一个 Linux epoll / muduo 风格的简化版 Reactor 网络库，源码保留在 `src_epoll/`，用于底层原理复习和与 Boost.Asio 主线做对照。

核心内容：

- Boost.Asio 异步 TCP 编程
- `io_context`、`tcp::acceptor`、`tcp::socket`
- `async_accept`、`async_read_some`、`async_read_until`、`async_write`
- session 生命周期管理
- 多线程 `io_context`
- HTTP 请求解析和静态响应
- Keep-Alive 基础支持
- CMake、测试、压测、Sanitizer 和阶段复盘

里程碑：

1. 项目基线：CMake、文档、脚本、Git。
2. 技术切换：旧 epoll 源码移动到 `src_epoll/`，新建 Boost.Asio 主线。
3. Echo Server：完成异步 TCP 回显服务。
4. HTTP Server：完成请求行、Header 解析和响应序列化。
5. 多线程运行：通过线程数参数控制 `io_context` 工作线程。
6. 测试补齐：CTest、手动 curl/nc、Windows/Linux 构建验证。
7. HTTP 完善：静态文件、错误页、超时策略。
8. 压测与调优：wrk/ab、连接数、吞吐和延迟记录。
9. 阶段复盘：总结 Boost.Asio 与手写 epoll 的差异。

## 第二阶段：WebSocket 长连接服务器

目标：基于第一阶段网络层，构建长连接服务能力。

核心内容：

- WebSocket 握手与帧编解码
- 心跳与超时
- 房间与广播
- 单聊与群聊
- 断线重连与 ACK
- 简单鉴权

## 第三阶段：KV Store

目标：实现一个类 Redis 的内存 KV 服务。

核心内容：

- TCP 协议设计
- GET、SET、DEL、EXPIRE
- TTL 与 TimerQueue 复用
- 快照或追加日志持久化
- 简单 LRU
- 慢查询日志

## 第四阶段：消息队列

目标：实现一个简单的 topic 型消息队列。

核心内容：

- producer 与 consumer
- ACK 与重试
- 死信队列
- 顺序消费
- 持久化日志
- consumer group 基础

## 第五阶段：RPC 框架

目标：补齐服务间通信能力。

核心内容：

- 编解码与 request id
- 超时与错误码
- 同步调用与异步调用
- 服务注册
- 简单负载均衡

## 后续实验

- C++20 协程异步 API
- ET 与 LT 行为和性能对比
- lock-free 队列评估
- Boost.Asio 跨平台原型
- Seastar 阅读笔记
