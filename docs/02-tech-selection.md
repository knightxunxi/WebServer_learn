# 技术选型

## C++ 标准

使用 C++20。

选择原因：

- 新项目可以使用更现代的标准库能力。
- `std::jthread`、`std::stop_token`、`std::span` 和 chrono 改进有实际价值。
- C++20 适合作为学习型和 GitHub 展示型项目的基线。

协程决策：

- 第一阶段主路径不使用协程。
- 核心网络模型保持 callback-style Reactor。
- 在连接生命周期、事件循环和回调模型清晰后，再单独加入协程实验模块。

## 平台选择

第一阶段只支持 Linux。

选择原因：

- Linux 是 C++ 服务端和基础架构方向的主要运行环境。
- epoll、fd 生命周期、非阻塞 IO 和事件驱动模型需要直接学习。
- 跨平台抽象可以在后续通过 Boost.Asio 单独评估。

## IO 后端

使用 epoll。

默认模式：

- LT + 非阻塞 fd

预留扩展：

- ET 配置模式

取舍说明：

- LT 更适合第一版稳定验证。
- ET 有学习价值，但必须严格处理读写循环直到 `EAGAIN`。
- 后续同时支持 LT/ET，可以进行行为和压测对比。

## 并发模型

使用 one loop per thread。

计划模型：

- main loop 负责接收新连接。
- sub loop 负责已建立连接的 IO。
- 每个 `EventLoop` 只属于一个线程。
- 跨线程任务投递使用 pending functor queue + eventfd 唤醒。

## 第三方依赖

第一阶段尽量保持依赖最小化。

允许：

- CMake
- Linux 系统 API
- 后续可选 GoogleTest

第一阶段暂不引入：

- Boost.Asio
- spdlog
- fmt
- 完整 HTTP parser 库

## 测试工具

计划使用：

- CTest：基础测试执行。
- curl、nc、telnet：手动网络行为验证。
- wrk 或 ab：压测。
- ASan、UBSan、Valgrind：内存和未定义行为检查。
- gdb、strace、lsof、perf：调试和性能分析。

