# 需求分析

## 阶段名称

`Boost.Asio + WebServer`

## 项目目的

当前阶段不是再次复制一个开源 WebServer，而是围绕 Boost.Asio 做一次跨平台 C++ 服务端工程化训练。最终用 TCP Echo Server 和小型 HTTP WebServer 验证异步网络、连接生命周期、协议解析、测试和文档流程。

此前完成的 Linux epoll / muduo 风格 Reactor 实现保留在 `src_epoll/`，作为底层原理学习和架构对照材料。

## 功能范围

必须完成：

- 在 Windows/Linux 上构建和运行。
- 使用 Boost.Asio 实现异步 TCP 服务。
- 支持 TCP Echo Server。
- 支持 HTTP WebServer。
- 支持多线程 `io_context`。
- 支持基础 HTTP/1.1 请求解析。
- 支持静态响应或静态文件响应。
- 支持 Keep-Alive。
- 提供 CMake 构建和脚本。
- 记录单元测试、集成测试和手动测试结果。
- 记录压测结果。

第一阶段不做：

- HTTPS/TLS。
- HTTP/2。
- 完整 Web 框架能力。
- 生产级日志系统。
- 分布式部署。
- 以协程作为主网络模型。
- 完整 HTTP body 解析。
- 生产级 HTTP parser。
- 生产级路由框架。

## 非功能需求

- 代码应保持可读、可解释。
- 核心所有权和生命周期规则必须文档化。
- 网络行为需要通过测试或手动验证记录。
- 重要技术选择记录到 `docs/02-tech-selection.md`。
- 每个里程碑更新 `docs/05-development-log.md`。
- 必要代码注释使用中文，重点解释设计意图和并发边界。

## 完成标准

- `csl_smoke` 可以构建和运行。
- Boost.Asio Echo Server 可以构建和运行。
- Boost.Asio HTTP WebServer 可以构建和运行。
- 主线模块具备必要测试。
- 记录至少一次 `wrk` 或 `ab` 压测结果。
- 记录至少一次 Sanitizer 或 Valgrind 检查结果。
- 在 `docs/08-review.md` 写完第一阶段复盘。
