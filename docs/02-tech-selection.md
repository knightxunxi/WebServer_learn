# 技术选型

## 当前结论

项目主线切换为 **Boost.Asio 跨平台异步网络实现**。

旧 Linux epoll / muduo 风格 Reactor 实现保留在 `src_epoll/`，定位为底层原理学习、架构对照和后续复盘材料，不再作为默认构建入口。

## C++ 标准

使用 C++20。

选择原因：

- 新项目可以使用更现代的标准库能力。
- C++20 适合作为学习型和 GitHub 展示型项目的基线。
- 后续可以独立扩展协程实验，但当前主线先使用 callback-style async IO，降低架构复杂度。

协程决策：

- 当前主路径不使用协程。
- 先把异步读写、连接生命周期、线程模型和测试流程跑通。
- 后续单独新增协程版本 API，与回调版本做复杂度和性能对比。

## 平台选择

当前主线支持 Windows/Linux。

选择 Boost.Asio 的原因：

- 统一封装 Windows IOCP、Linux epoll 等平台差异。
- 在 Windows 本地开发、调试和演示更方便。
- 能把学习重点放在异步模型、连接管理、协议解析、测试和工程流程上。
- 后续在 Linux 上仍可运行和压测，方便与历史 epoll 实现做对照。

保留 `src_epoll/` 的原因：

- epoll、eventfd、timerfd、非阻塞 fd 生命周期仍然是 C++ 服务端的重要基础。
- 旧实现已经覆盖 Channel、Poller、EventLoop、TcpConnection、TimerQueue、HttpServer 等模块。
- 后续可以用于复习底层原理，也可以作为简历中“理解底层事件驱动机制”的证据。

## IO 后端

当前主线使用 Boost.Asio。

默认模型：

- `boost::asio::io_context`
- `tcp::acceptor`
- `tcp::socket`
- `async_accept`
- `async_read_some` / `async_read_until`
- `async_write`

历史 epoll 版本：

- 默认 LT + 非阻塞 fd。
- ET 配置模式保留为后续对照实验方向。
- 不参与 Windows 默认构建。

## 并发模型

当前主线使用多线程 `io_context`。

计划模型：

- 一个 `io_context` 负责接收连接和分发 IO 事件。
- 根据命令行参数启动 N 个工作线程。
- 每个连接由 session 对象持有 socket 和读缓冲区。
- session 使用 `shared_from_this()` 保证异步回调期间对象存活。

与旧 one loop per thread 模型的区别：

- 旧 epoll 版本强调手写 Reactor 组件和线程归属。
- Boost.Asio 版本把事件分发交给库处理，更适合跨平台和快速构建应用层能力。

## 第三方依赖

当前主线依赖：

- CMake
- Boost.Asio
- C++20 编译器
- 系统线程库

Windows 额外链接：

- `ws2_32`
- `mswsock`

暂不引入：

- spdlog
- fmt
- 完整 HTTP parser 库
- GoogleTest

暂不引入这些库的原因是保持当前阶段可解释、可调试，避免项目过早变成依赖集成练习。

## 测试工具

计划使用：

- CTest：基础测试执行。
- curl、nc、telnet：手动网络行为验证。
- wrk 或 ab：Linux 压测。
- Sanitizer、Valgrind：Linux 内存和未定义行为检查。
- Visual Studio Debugger：Windows 本地调试。
- gdb、strace、lsof、perf：Linux 调试和性能分析。

## 当前取舍

这次切换不是放弃底层网络学习，而是把项目主线从“手写 Linux 网络库”调整为“可跨平台构建的 C++ 服务端工程”。

简历和 GitHub 展示时可以这样表达：

- 已实现过 Linux epoll Reactor，用于理解底层事件驱动、非阻塞 IO 和连接生命周期。
- 当前项目主线基于 Boost.Asio，面向 Windows/Linux 跨平台服务端开发。
- 后续重点放在 HTTP 完善、WebSocket 长连接、测试流程、压测和工程化文档。
