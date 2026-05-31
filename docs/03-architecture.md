# 架构设计

第一阶段采用 muduo 风格 Reactor 架构，但实现范围控制在适合学习和维护的规模内。

## 分层结构

```text
apps
  web_server / echo_server
http
  HttpServer / HttpContext / HttpRequest / HttpResponse
net
  TcpServer / TcpConnection / Acceptor / EventLoop / Channel / Poller
timer
  TimerQueue / Timer / TimerId
base
  Logger / Timestamp / noncopyable
```

## 主运行模型

```text
main thread
  EventLoop
    Acceptor
      accept 新连接
      将连接分配给 sub EventLoop

worker thread N
  EventLoop
    TcpConnection
      read
      decode
      callback
      write
      close
```

## 重要所有权规则

- `EventLoop` 只属于一个线程。
- `Channel` 不拥有 fd，只保存 fd 的事件兴趣和回调。
- `Poller` 拥有 epoll 实例，并负责收集活跃事件。
- `TcpConnection` 负责连接生命周期抽象。
- `Socket` 封装 fd 操作，并在合适位置通过 RAII 关闭 fd。
- `Buffer` 管理读写内存，隐藏半包、粘包和部分写细节。
- `TimerQueue` 可被 HTTP 超时、WebSocket 心跳、KV TTL 和 MQ 延迟任务复用。

## 连接生命周期

第一阶段目标生命周期：

```text
accept
  -> 设置非阻塞
  -> 创建 TcpConnection
  -> 注册读事件
  -> 读取数据到 Buffer
  -> 调用 message callback
  -> 写响应或暂存到 output buffer
  -> 处理对端关闭 / 错误 / 超时
  -> 移除 Channel
  -> 销毁 TcpConnection
```

## 触发模式策略

架构上预留：

```text
TriggerMode::Level
TriggerMode::Edge
```

先实现并稳定 LT。等读写路径、错误处理和测试足够稳定后，再加入 ET。

## 协程定位

协程不属于第一阶段核心架构。后续可以用实验模块包装 callback 异步操作，探索 `co_await` 风格 API，但不应影响 Reactor 基线。

