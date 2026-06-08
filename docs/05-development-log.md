# 开发日志

## 2026-05-31

初始化仓库规划和工程骨架。

已确定：

- 仓库定位为长期 C++ 服务端学习项目。
- 第一阶段为 `MiniMuduo + WebServer`。
- 使用 C++20。
- 第一阶段使用 Linux epoll。
- 默认触发模式为 LT。
- ET 作为后续可配置扩展。
- 第一阶段主线使用 callback-style Reactor。
- 协程作为后续实验模块。
- 文档在开发过程中持续更新，不等到项目结束后补写。

已创建：

- README
- 学习路线、需求分析、技术选型、架构设计、模块设计、测试方案、工作流文档
- CMake 基线
- smoke app 和 smoke test
- 构建和测试脚本

## 2026-05-31

统一文档和注释语言。

已确定：

- README 与 `docs/` 文档使用中文。
- 后续必要代码注释使用中文。
- 注释重点解释设计意图、生命周期、并发边界和容易踩坑的位置。
- 不为显而易见的代码添加无意义注释。

## 2026-06-02

产出里程碑 #2 详细执行计划。

已确定：

- 引入企业级里程碑微循环流程：需求拆解 → 接口设计 → 编码实现 → 测试验证 → 记录复盘。
- 参考 muduo 源码 `D:\C1\git_open_resource\muduo-master` 的 base 模块接口设计。
- 里程碑 #2 目标：实现 `noncopyable`、`Timestamp`、`Logger` 三个基础工具类。
- 与 muduo 的关键差异：无 Boost 依赖、使用 Pimpl + std::ostringstream、C++20 标准。
- 步骤拆分为 4 个子步骤：noncopyable (0.5h) → Timestamp (1.5h) → Logger (2h) → 收尾 (0.5h)。

已创建：

- `docs/12-milestone-02-plan.md`：里程碑 #2 执行计划，包含接口设计、验收标准、文件清单。

完成里程碑 #2 基础工具模块。

已创建（9+3 个文件）：

- `include/csl/base/noncopyable.h` / `tests/base/noncopyable_test.cpp`
- `include/csl/base/timestamp.h` / `src/base/timestamp.cpp` / `tests/base/timestamp_test.cpp`
- `include/csl/base/logger.h` / `src/base/logger.cpp` / `tests/base/logger_test.cpp`
- `tests/CMakeLists.txt`（新增 3 个测试 target）

已修复（QA 审查发现的 Bug）：

- BUG-1：`Timestamp::toString()` 负数微秒格式化异常（C++ 取模符号问题）
- BUG-2：`Logger::Impl::finish()` snprintf 截断时返回值超出 buf 大小
- BUG-3：`Timestamp::toFormattedString()` 负数时间戳 gmtime_r 未定义行为

完成里程碑 #3 事件循环基线。

已创建（9 个文件 + 1 测试）：

- `include/csl/net/Channel.h` / `src/net/Channel.cpp`
- `include/csl/net/Poller.h` / `src/net/Poller.cpp`
- `include/csl/net/EpollPoller.h` / `src/net/EpollPoller.cpp`
- `include/csl/net/EventLoop.h` / `src/net/EventLoop.cpp`
- `docs/13-milestone-03-plan.md`
- `tests/net/eventloop_test.cpp`
- `tests/CMakeLists.txt`（新增 eventloop_test target）

核心设计：
- EventLoop：one loop per thread，poll + 事件分发 + pending functors + eventfd 唤醒
- Channel：fd 事件绑定、回调注册、handleEvent 事件分发
- Poller/EpollPoller：IO 多路复用抽象 + epoll LT 实现
- 定时器接口暂时移除（里程碑 #8 再加入）

完成里程碑 #4 TCP 基线。

已创建（8 个文件 + 1 测试）：

- `include/csl/net/InetAddress.h` / `src/net/InetAddress.cpp`
- `include/csl/net/Socket.h` / `src/net/Socket.cpp`
- `include/csl/net/Acceptor.h` / `src/net/Acceptor.cpp`
- `include/csl/net/TcpConnection.h` / `src/net/TcpConnection.cpp`
- `tests/net/tcp_test.cpp`
- `tests/CMakeLists.txt`（新增 tcp_test target）

核心设计：
- InetAddress：IPv4 地址封装，支持 "ip:port" 解析
- Socket：RAII socket fd，bind/listen/accept/setsockopt
- Acceptor：监听 socket + Channel，accept 新连接并通过回调分发
- TcpConnection：shared_from_this + Channel::tie()，message/close 回调，临时 inputBuffer_（里程 #6 替换为 Buffer）

完成里程碑 #5 Echo Server。

已创建（3 个文件）：

- `include/csl/net/TcpServer.h` / `src/net/TcpServer.cpp`（单线程版，管理连接表）
- `apps/echo_server/main.cpp`（监听 9999，echo 回显）
- `CMakeLists.txt`（新增 csl_echo_server target）

里程碑 #5 是第一个可运行的网络应用：nc localhost 9999 连接后输入任意内容即可收到 echo。

完成里程碑 #6 Buffer。

已创建（3 个文件）：

- `include/csl/net/Buffer.h`（header-only，prependable+readable+writable 三段布局）
- `src/net/Buffer.cpp`（readFd 使用 readv + 栈缓冲区避免频繁扩容）
- `tests/net/buffer_test.cpp`（6 项单元测试）
- `tests/CMakeLists.txt`（新增 buffer_test target）

完成里程碑 #7 多线程 Reactor。

已创建（4 个文件 + 升级 2 个）：

- `include/csl/net/EventLoopThread.h` / `src/net/EventLoopThread.cpp`
- `include/csl/net/EventLoopThreadPool.h` / `src/net/EventLoopThreadPool.cpp`
- 升级 `TcpServer`：支持 setThreadNum()，通过 round-robin 分发连接
- 升级 `echo_server`：默认 2 个 IO 线程

完成里程碑 #8 TimerQueue。

已创建（3 个文件）：

- `include/csl/timer/Timer.h`（一次性/重复定时器，atomic 序号）
- `include/csl/timer/TimerId.h`（轻量取消句柄）
- `include/csl/timer/TimerQueue.h` / `src/timer/TimerQueue.cpp`（timerfd + std::set<Entry>）
- 升级 `EventLoop`：集成 TimerQueue，暴露 runAt/runAfter/runEvery/cancel

完成里程碑 #9 HTTP Server。

已创建（5 个文件）：

- `include/csl/http/HttpRequest.h` / `src/http/HttpRequest.cpp`
- `include/csl/http/HttpResponse.h`
- `include/csl/http/HttpContext.h`（HTTP/1.1 请求行+头部解析）
- `include/csl/http/HttpServer.h` / `src/http/HttpServer.cpp`（构建在 TcpServer 上）
- `apps/web_server/main.cpp`（监听 8080，返回 HTML 页面）

---

## 第一阶段里程碑 #10 收尾

第一阶段 `MiniMuduo + WebServer` 全部 10 个里程碑已完成。

最终交付：

```
include/csl/
  base/    noncopyable.h  timestamp.h  logger.h
  net/     Buffer.h  Channel.h  EventLoop.h  Poller.h  EpollPoller.h
           Socket.h  InetAddress.h  Acceptor.h  TcpConnection.h
           TcpServer.h  EventLoopThread.h  EventLoopThreadPool.h
  timer/   Timer.h  TimerId.h  TimerQueue.h
  http/    HttpRequest.h  HttpResponse.h  HttpContext.h  HttpServer.h
src/
  base/    timestamp.cpp  logger.cpp
  net/     Channel.cpp  Poller.cpp  EpollPoller.cpp  EventLoop.cpp
           Socket.cpp  InetAddress.cpp  Acceptor.cpp  TcpConnection.cpp
           TcpServer.cpp  EventLoopThread.cpp  EventLoopThreadPool.cpp  Buffer.cpp
  timer/   TimerQueue.cpp
  http/    HttpRequest.cpp  HttpServer.cpp
apps/
  smoke/main.cpp    echo_server/main.cpp    web_server/main.cpp
tests/
  base/        noncopyable_test.cpp  timestamp_test.cpp  logger_test.cpp
  net/         eventloop_test.cpp  tcp_test.cpp  buffer_test.cpp
docs/
  00-roadmap.md ~ 13-milestone-03-plan.md（共 14 份文档）
CMakeLists.txt（3 个 app target + 6 个 test target）
```

总计 50 个代码文件，覆盖 Reactor 全栈。
