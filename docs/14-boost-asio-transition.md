# Boost.Asio 主线切换记录

## 背景

项目最初以 Linux epoll / muduo 风格 Reactor 为第一阶段目标，已经完成了 EventLoop、Channel、Poller、TcpConnection、TcpServer、TimerQueue 和 HTTP Server 等模块。

为了后续在 Windows 平台更方便地开发、调试和演示，同时保持 Linux 可运行能力，项目主线切换为 Boost.Asio。

## 目录调整

当前目录策略：

```text
src/
  asio/          当前 Boost.Asio 主线实现

src_epoll/       旧 Linux epoll / MiniMuduo 实现
```

头文件策略：

```text
include/csl/asio/    当前主线对外接口
include/csl/net/     旧 epoll 网络模块头文件
include/csl/http/    旧 epoll HTTP 模块头文件
include/csl/base/    旧基础工具模块头文件
include/csl/timer/   旧定时器模块头文件
```

## 当前主线模块

当前 Boost.Asio 主线包含：

- `EchoServer`：异步 TCP 回显服务器。
- `HttpServer`：基础 HTTP/1.1 服务器。
- `HttpRequest`：请求行与基础头部解析结果。
- `HttpResponse`：响应状态、内容类型、响应体和连接关闭策略。
- `serializeResponse()`：HTTP 响应序列化函数。

当前暂不包含：

- 完整 HTTP body 解析。
- 静态文件服务。
- 路由器。
- TLS。
- WebSocket。
- 协程 API。

## 构建策略

默认构建：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCSL_BUILD_TESTS=ON
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure
```

默认目标：

- `csl_smoke`
- `csl_echo_server`
- `csl_web_server`
- `csl_smoke_test`
- `csl_asio_http_response_test`

旧 epoll 测试：

```bash
cmake -S . -B build-epoll -DCMAKE_BUILD_TYPE=Debug -DCSL_BUILD_TESTS=ON -DCSL_BUILD_LEGACY_EPOLL=ON
cmake --build build-epoll -j"$(nproc)"
ctest --test-dir build-epoll --output-on-failure
```

注意：旧 epoll 测试依赖 `epoll`、`eventfd`、`timerfd`，仅面向 Linux。

## 后续任务

短期任务：

- 补充 HTTP 静态文件响应。
- 增加连接超时策略。
- 增加更多 Asio 单元测试和手动测试记录。
- 在 Windows 与 Linux 各完成一次构建验证。

中期任务：

- 增加 WebSocket 长连接服务。
- 记录 Boost.Asio 与手写 epoll Reactor 的架构差异。
- 进行 wrk/ab 压测并记录吞吐、延迟和资源占用。

长期任务：

- 评估 C++20 协程版 Asio API。
- 与历史 epoll 实现做性能和复杂度对比。
