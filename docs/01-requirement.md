# 需求分析

## 阶段名称

`MiniMuduo + WebServer`

## 项目目的

第一阶段不是再次复制一个开源 WebServer，而是围绕 muduo 风格 Reactor 网络库做一次完整工程化训练。最终用一个小型 HTTP WebServer 作为应用层验证网络库能力。

## 功能范围

必须完成：

- 在 Linux 上运行。
- 使用非阻塞 socket fd。
- 使用 epoll 作为 IO 多路复用后端。
- 默认使用 LT 触发模式。
- LT 稳定后，支持通过配置切换 ET。
- 先完成单线程 Echo Server。
- 后续支持多线程 Reactor。
- 支持基础 HTTP/1.1 请求解析。
- 支持静态响应或静态文件响应。
- 支持 Keep-Alive。
- 通过 TimerQueue 支持连接超时。
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
- Windows 支持。
- Boost.Asio 实现。

## 非功能需求

- 代码应保持可读、可解释。
- 核心所有权和生命周期规则必须文档化。
- 网络行为需要通过测试或手动验证记录。
- 重要技术选择记录到 `docs/02-tech-selection.md`。
- 每个里程碑更新 `docs/05-development-log.md`。
- 必要代码注释使用中文，重点解释设计意图和并发边界。

## 完成标准

- `csl_smoke` 可以构建和运行。
- Echo Server 里程碑可以构建和运行。
- HTTP WebServer 里程碑可以构建和运行。
- 核心模块具备必要测试。
- 记录至少一次 `wrk` 或 `ab` 压测结果。
- 记录至少一次 Sanitizer 或 Valgrind 检查结果。
- 在 `docs/08-review.md` 写完第一阶段复盘。

