# C++ Server Lab

`cpp-server-lab` 是一个面向 C++ 服务端能力建设的学习型仓库。当前主线切换为 **Boost.Asio 跨平台异步网络服务**，用于在 Windows 和 Linux 上统一构建、运行 Echo Server 与 HTTP WebServer。

历史 Linux epoll / muduo 风格 Reactor 实现已保留在 `src_epoll/`，作为网络底层原理学习、接口设计对照和后续复盘材料。

这个仓库不是为了堆项目数量，而是按照接近企业开发的流程，把需求分析、技术选型、架构设计、模块实现、测试、压测、问题记录和阶段复盘都沉淀下来，方便后续回顾和 GitHub 展示。

## 学习目标

- 从需求到复盘，完整走一遍 C++ 服务端开发流程。
- 使用 Boost.Asio 实现跨平台异步 TCP/HTTP 服务。
- 理解 Reactor、非阻塞 IO、连接生命周期、多线程 `io_context` 和回调式异步模型。
- 保留 Linux epoll 实现，用于对照学习底层事件驱动模型。
- 持续记录技术决策、问题定位、测试计划和压测结果。
- 保证项目可以上传 GitHub，并能在 Windows/Linux 中 clone、构建、运行。

## 当前阶段

第一阶段主线：`Boost.Asio + WebServer`

- 语言标准：C++20
- 平台定位：Windows/Linux
- IO 模型：Boost.Asio
- 并发模型：多线程 `io_context`
- 核心风格：callback-style async IO
- 当前应用：TCP Echo Server、HTTP WebServer
- 旧实现：`src_epoll/` 中保留 Linux epoll 版本
- 协程定位：后续实验模块，不进入第一阶段主路径

## 仓库结构

```text
.
├── apps/                         # 可运行示例和服务程序
│   ├── echo_server/
│   ├── smoke/
│   └── web_server/
├── benchmarks/                   # 压测脚本和压测结果
├── config/                       # WebServer 默认配置
├── docs/                         # 需求、设计、日志、测试、复盘文档
├── experiments/                  # 实验模块，例如协程版本 API
├── include/csl/                  # 对外头文件
│   ├── asio/                     # Boost.Asio 主线接口
│   ├── base/
│   ├── http/
│   ├── net/
│   └── timer/
├── public/                       # 静态资源目录
├── scripts/                      # 构建、测试、压测辅助脚本
├── src/                          # 当前主线实现
│   └── asio/
├── src_epoll/                    # 旧 Linux epoll / MiniMuduo 实现
└── tests/                        # 单元测试和集成测试
```

## Linux 构建

依赖：

```bash
sudo apt update
sudo apt install -y build-essential cmake git libboost-all-dev
```

构建：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCSL_BUILD_TESTS=ON
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure
```

运行：

```bash
./build/csl_echo_server 9999 2
./build/csl_web_server config/server.ini
```

如果进入构建目录运行：

```bash
cd build
./csl_web_server ../config/server.ini
```

## Windows 构建

需要安装：

- CMake
- Visual Studio 2022 或其他 C++20 编译器
- Boost（可通过 vcpkg、MSYS2 或手动安装）

vcpkg 示例：

```powershell
cmake -S . -B build -DCSL_BUILD_TESTS=ON -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

运行：

```powershell
.\build\Debug\csl_echo_server.exe 9999 2
.\build\Debug\csl_web_server.exe config\server.ini
```

如果进入构建目录运行：

```powershell
cd build-utf8-check
.\csl_web_server.exe ..\config\server.ini
```

WebServer 默认能力：

- 读取 `config/server.ini`。
- 从 `public/` 返回静态文件。
- 支持 `GET /`、`GET /style.css`、`GET /api/status`。
- 支持 `POST /api/echo`，用于验证基础 body 读取。
- 运行日志输出到 `logs/server.log`。

也可以使用脚本：

```bash
bash scripts/build.sh
bash scripts/test.sh
```

## 文档索引

- [学习路线](docs/00-roadmap.md)
- [需求分析](docs/01-requirement.md)
- [技术选型](docs/02-tech-selection.md)
- [架构设计](docs/03-architecture.md)
- [模块设计](docs/04-module-design.md)
- [开发日志](docs/05-development-log.md)
- [问题记录](docs/06-problems.md)
- [压测记录](docs/07-benchmark.md)
- [阶段复盘](docs/08-review.md)
- [测试方案](docs/09-testing.md)
- [GitHub 与 Linux 工作流](docs/10-github-linux-workflow.md)
- [编码与文档规范](docs/11-style-guide.md)
- [Boost.Asio 主线切换记录](docs/14-boost-asio-transition.md)

## 开发流程

1. 实现重要模块前，先更新需求或设计文档。
2. 每次只推进一个可以验证的小里程碑。
3. 对确定性逻辑补单元测试，对网络行为补集成或手动测试。
4. 遇到 bug、理解卡点和技术取舍，记录到 `docs/06-problems.md`。
5. 完成里程碑前运行构建、测试和必要压测。
6. 每个阶段结束后写复盘，明确可复用内容和后续改进点。

## 注释原则

后续必要代码注释统一使用中文。注释只解释不明显的设计意图、生命周期约束、并发边界和关键坑点，不为显而易见的代码写空泛注释。
