# 学习路线

本仓库作为长期 C++ 服务端学习项目维护。每个阶段都应包含需求分析、架构设计、代码实现、测试验证、压测记录和阶段复盘。

## 第一阶段：MiniMuduo + WebServer

目标：实现一个 muduo 风格的简化版 Reactor 网络库，并基于它开发 HTTP WebServer。

核心内容：

- Linux 非阻塞网络编程
- epoll 默认 LT，后续支持 ET 配置切换
- Reactor 与 one loop per thread
- Channel、Poller、EventLoop、Acceptor、TcpConnection、TcpServer
- Buffer 与连接生命周期管理
- TimerQueue 与超时处理
- HTTP 请求解析和静态响应
- CMake、测试、压测、Sanitizer 和阶段复盘

里程碑：

1. 项目基线：CMake、文档、脚本、Git。
2. 基础工具：noncopyable、Timestamp、Logger。
3. 事件循环基线：EventLoop、Channel、Poller、EpollPoller。
4. TCP 基线：Socket、InetAddress、Acceptor、TcpConnection。
5. Echo Server：先完成单线程可运行版本。
6. Buffer 与写路径：处理半包、粘包和部分写。
7. 多线程 Reactor：EventLoopThread 与 EventLoopThreadPool。
8. TimerQueue：支持超时和定时任务。
9. HTTP Server：解析请求并返回响应。
10. 测试、压测、复盘和 README 完善。

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

