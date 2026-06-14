# 问题记录

这里记录 bug、理解卡点、调试过程和最终结论。

## 模板

```text
## YYYY-MM-DD：简短标题

背景：

现象：

原因：

修复：

如何预防：
```

## 2026-06-08：Timer 编译级问题

背景：
里程碑 #8 引入 `Timer` 和 `TimerQueue`。

现象：
`Timer` 中存在 `const int64_t sequence_`，但构造函数没有初始化；同时头文件使用 `std::atomic` 却没有包含 `<atomic>`。

原因：
实现 Timer 序号时只添加了静态计数器，没有完成每个 Timer 实例的序号初始化。

修复：
在 `Timer` 构造函数中使用 `s_numCreated_.fetch_add(1) + 1` 初始化 `sequence_`，并补齐 `<atomic>` 和 `<utility>`。

如何预防：
涉及 `const` 成员和静态计数器时，优先补单元测试或至少进行 Linux 编译验证。

## 2026-06-14：Windows 控制台中文输出乱码

背景：
项目主线切换到 Boost.Asio 后，在 Windows CMD 中运行 `csl_web_server.exe`。

现象：
控制台输出出现 `鐩戝惉绔彛`、`璇锋眰` 等乱码。

原因：
源码和程序输出使用 UTF-8 字节，但 Windows CMD 默认代码页可能是 GBK/936，控制台按错误编码解释中文。

修复：
新增 `csl/platform/console.h`，在 Windows 下调用 `SetConsoleOutputCP(CP_UTF8)` 和 `SetConsoleCP(CP_UTF8)`；同时 MinGW 构建显式增加 `-finput-charset=UTF-8` 与 `-fexec-charset=UTF-8`。

如何预防：
Windows 命令行程序如果需要输出中文，应同时控制源码编码、执行字符集和控制台代码页。临时方案可以在运行前执行 `chcp 65001`。

## 2026-06-08：TcpConnection 未真正接入 Buffer

背景：
里程碑 #6 已实现 `Buffer`，文档中也写明要用于处理半包、粘包和部分写。

现象：
`TcpConnection` 仍使用临时栈缓冲和 `std::string inputBuffer_`，发送路径也只是单次 `write()`。

原因：
Buffer 模块实现后没有同步改造 TcpConnection 的输入/输出路径。

修复：
将消息回调调整为 `MessageCallback(conn, Buffer*, Timestamp)`；读路径使用 `Buffer::readFd()`；写路径增加 `outputBuffer_`，未写完的数据注册写事件后续发送。

如何预防：
每个里程碑完成后检查“模块已实现”和“模块已接入”是否一致。

## 2026-06-08：HTTP 解析未使用 HttpContext

背景：
HTTP 层实现了 `HttpContext::parseRequest(Buffer*)`。

现象：
`HttpServer::onMessage()` 实际绕过 `HttpContext`，使用 `std::string` 简单解析请求行；Keep-Alive 和分包请求无法可靠处理。

原因：
为了快速跑通 WebServer，曾保留临时解析方案，后续没有及时替换。

修复：
`HttpServer::onMessage()` 改为使用连接上下文中的 `HttpContext` 解析 `Buffer`；`HttpContext` 改为逐行解析请求行和 header；响应后根据 `Connection` 和 HTTP 版本决定是否关闭连接。

如何预防：
临时方案必须在注释和开发日志中标记，并在阶段收尾前集中清理。

## 2026-06-08：EpollPoller 状态机边界问题

背景：
`Channel::disableAll()` 会触发 `Poller::updateChannel()`。

现象：
当 Channel 已经处于 `kDeleted` 且没有任何监听事件时，再次 `disableAll()` 可能导致 `EpollPoller` 以空事件重新 `ADD` 到 epoll。

原因：
`EpollPoller::updateChannel()` 没有处理 `index == kDeleted && isNoneEvent()` 的边界状态。

修复：
在 `index == -1 || index == kDeleted` 分支中，如果 `channel->isNoneEvent()`，直接返回，不重新注册空事件。

如何预防：
Channel/Poller 状态机需要覆盖“重复 disable”“disable 后 remove”等边界测试。

## 2026-06-08：测试用例与实现语义不一致

背景：
补充 `EventLoop` 和 TCP 集成测试。

现象：
`eventloop_test` 中 `runInLoop()` 在当前线程调用会立即执行，但测试断言其未执行；写事件测试把 pipe 读端当作写事件 fd；`tcp_test` 中连接对象是回调局部变量，回调结束后生命周期不足。

原因：
测试编写时没有完全对齐 EventLoop 线程归属语义和 TcpConnection 的 shared_ptr 生命周期模型。

修复：
修正 `runInLoop` 断言；写事件测试改为监听 pipe 写端；TCP 测试用外部 `shared_ptr` 持有连接，并在测试结束时显式 `connectDestroyed()`。

如何预防：
测试代码也需要做代码审查，尤其是涉及事件循环、fd 和 shared_ptr 生命周期的场景。
