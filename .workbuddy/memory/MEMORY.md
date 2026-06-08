# cpp-server-lab 项目记忆

## 技术栈
- C++20、CMake 3.20+、Linux only
- epoll LT 默认、one loop per thread、callback-style Reactor
- 参考 muduo（无 Boost 依赖）

## 当前阶段
- 第一阶段（MiniMuduo + WebServer）10 个里程碑全部完成
- 50 个代码文件，14 份文档

## 编码约定
- 命名空间 csl，文件命名 snake_case，类名 PascalCase
- 注释用中文，重点解释设计意图、生命周期、并发边界
- Windows 环境仅编码不编译，上传 GitHub 后在 Linux 端验证
- 企业级微循环流程：需求拆解 → 接口设计 → 编码 → 测试 → 复盘
