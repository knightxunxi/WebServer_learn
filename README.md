# C++ Server Lab

`cpp-server-lab` 是一个面向 C++ 服务端能力建设的学习型仓库。第一阶段以 muduo 风格的 Reactor 架构为参考，实现一个简化版网络库，并在其上开发 HTTP WebServer。

这个仓库不是为了堆项目数量，而是按照接近企业开发的流程，把需求分析、技术选型、架构设计、模块实现、测试、压测、问题记录和阶段复盘都沉淀下来，方便后续回顾和 GitHub 展示。

## 学习目标

- 从需求到复盘，完整走一遍 Linux C++ 服务端开发流程。
- 理解 Reactor、epoll、非阻塞 IO、连接生命周期、定时器和 one loop per thread。
- 持续记录技术决策、问题定位、测试计划和压测结果。
- 保证项目可以上传 GitHub，并能在 Linux 虚拟机中 clone、构建、运行。

## 当前阶段

第一阶段：`MiniMuduo + WebServer`

- 语言标准：C++20
- 平台定位：Linux 优先
- IO 模型：epoll + 非阻塞 fd
- 触发模式：默认 LT，预留 ET 配置扩展
- 并发模型：one loop per thread
- 核心风格：callback-style Reactor
- 协程定位：后续实验模块，不进入第一阶段主路径

## 仓库结构

```text
.
├── apps/                         # 可运行示例和服务程序
│   ├── echo_server/
│   ├── smoke/
│   └── web_server/
├── benchmarks/                   # 压测脚本和压测结果
├── docs/                         # 需求、设计、日志、测试、复盘文档
├── experiments/                  # 实验模块，例如协程版本 API
├── include/csl/                  # 对外头文件
│   ├── base/
│   ├── http/
│   ├── net/
│   └── timer/
├── scripts/                      # 构建、测试、压测辅助脚本
├── src/                          # 具体实现
└── tests/                        # 单元测试和集成测试
```

## Linux 构建

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCSL_BUILD_TESTS=ON
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure
```

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

## 开发流程

1. 实现重要模块前，先更新需求或设计文档。
2. 每次只推进一个可以验证的小里程碑。
3. 对确定性逻辑补单元测试，对网络行为补集成或手动测试。
4. 遇到 bug、理解卡点和技术取舍，记录到 `docs/06-problems.md`。
5. 完成里程碑前运行构建、测试和必要压测。
6. 每个阶段结束后写复盘，明确可复用内容和后续改进点。

## 注释原则

后续必要代码注释统一使用中文。注释只解释不明显的设计意图、生命周期约束、并发边界和关键坑点，不为显而易见的代码写空泛注释。

