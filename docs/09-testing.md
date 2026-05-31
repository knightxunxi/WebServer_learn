# 测试方案

第一阶段需要把测试流程补上，因为这个仓库的目标是按照工程流程学习，而不是只写出代码。

## 测试层级

单元测试：

- Buffer
- HTTP parser
- TimerQueue
- 如果加入配置解析，也需要测试配置解析

集成测试：

- Echo Server 单连接
- Echo Server 多连接
- HTTP 请求与响应
- Keep-Alive
- 客户端异常断开
- 服务端主动关闭

手动测试：

- `curl`
- `nc`
- `telnet`
- 浏览器访问静态响应

压测：

- `wrk`
- `ab`

调试和质量检查：

- ASan
- UBSan
- Valgrind
- gdb
- strace
- lsof
- perf

## 当前基线

初始仓库已经通过 CTest 放置了一个 smoke test：

```bash
cmake -S . -B build -DCSL_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## 第一批测试任务

1. Buffer 实现后补 Buffer 单元测试。
2. EventLoop 与 Channel 实现后补 EventLoop smoke 集成测试。
3. 第一个 Echo Server 可运行后补手动测试记录。
4. HTTP Server 暴露前先补 HTTP parser 单元测试。

