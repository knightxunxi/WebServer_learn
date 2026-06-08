# 里程碑 #3 执行计划：事件循环基线

> 版本：v1.0 | 日期：2026-06-02  
> 参考：muduo net/EventLoop、Channel、Poller、EpollPoller

---

## 目标

实现 four 个核心类，建立 Reactor 事件循环框架：

| 类 | 职责 | 生命周期 |
|---|---|---|
| `Channel` | 封装 fd 的事件兴趣和回调 | 不拥有 fd，绑定到 EventLoop |
| `Poller` | IO 多路复用抽象接口 | 被 EventLoop 独占 |
| `EpollPoller` | Linux epoll 具体实现 | 拥有 epoll fd |
| `EventLoop` | 单线程事件循环中枢 | 每个线程最多一个 |

---

## 文件清单（10 个文件）

### 新建（9 个）
```
include/csl/net/Channel.h
include/csl/net/EventLoop.h
include/csl/net/Poller.h
include/csl/net/EpollPoller.h
src/net/Channel.cpp
src/net/EventLoop.cpp
src/net/Poller.cpp
src/net/EpollPoller.cpp
tests/net/eventloop_test.cpp
```

### 修改（1 个）
```
tests/CMakeLists.txt
```

---

## 接口设计

### Channel

```cpp
namespace csl {

class EventLoop;

class Channel : noncopyable {
public:
    // 事件常量
    static const int kNoneEvent  = 0;
    static const int kReadEvent  = EPOLLIN | EPOLLPRI;
    static const int kWriteEvent = EPOLLOUT;

    // 回调类型
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop* loop, int fd);
    ~Channel();

    // 回调设置
    void setReadCallback(ReadEventCallback cb);
    void setWriteCallback(EventCallback cb);
    void setCloseCallback(EventCallback cb);
    void setErrorCallback(EventCallback cb);

    // 事件控制（每个操作后自动 update）
    void enableReading();
    void disableReading();
    void enableWriting();
    void disableWriting();
    void disableAll();

    // getter
    int fd() const;
    int events() const;
    int index() const;          // Poller 使用的索引
    void set_index(int idx);

    // 仅供 Poller 调用
    void set_revents(int revt);
    int revents() const;        // CJL 新增：便于测试

    // 事件分发
    void handleEvent(Timestamp receiveTime);

    // 从 EventLoop 移除
    void remove();

    EventLoop* ownerLoop() const;

    // 生命周期绑定（防止 handleEvent 中 owner 被销毁）
    void tie(const std::shared_ptr<void>& obj);

    // 状态查询
    bool isNoneEvent() const;
    bool isReading() const;
    bool isWriting() const;

private:
    void update();              // 通知 EventLoop 更新监听

    EventLoop* loop_;
    int fd_;
    int events_;                // 关心的事件
    int revents_;               // Poller 返回的已就绪事件
    int index_;                 // Poller 使用

    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;

    std::weak_ptr<void> tie_;
    bool tied_;
    bool eventHandling_;
    bool addedToLoop_;
};
}
```

### EventLoop

```cpp
namespace csl {

class EventLoop : noncopyable {
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    void loop();    // 永久循环（阻塞）
    void quit();    // 退出循环（可跨线程调用）

    // 任务投递
    void runInLoop(Functor cb);
    void queueInLoop(Functor cb);
    size_t queueSize() const;

    // Channel 管理
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    // 定时器（骨架，TimerQueue 未实现前为空实现）
    TimerId runAt(Timestamp time, std::function<void()> cb);
    TimerId runAfter(double delay, std::function<void()> cb);
    TimerId runEvery(double interval, std::function<void()> cb);
    void cancel(TimerId timerId);

    // 线程检查
    bool isInLoopThread() const;
    void assertInLoopThread();

    // 唤醒 loop（用于跨线程 queueInLoop 后通知）
    void wakeup();

    // 静态
    static EventLoop* getEventLoopOfCurrentThread();

private:
    // 内部方法
    void handleRead();               // wakeupChannel 的读回调
    void doPendingFunctors();        // 执行待处理回调队列

    bool looping_;
    bool quit_;
    bool eventHandling_;
    bool callingPendingFunctors_;
    const std::thread::id threadId_;
    Timestamp pollReturnTime_;

    std::unique_ptr<Poller> poller_;

    // wakeup 机制（eventfd）
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;

    // 活跃 Channel 列表（poll 返回）
    using ChannelList = std::vector<Channel*>;
    ChannelList activeChannels_;
    Channel* currentActiveChannel_;

    // 待处理任务队列
    mutable std::mutex mutex_;
    std::vector<Functor> pendingFunctors_;
};
}
```

### Poller

```cpp
namespace csl {

class Poller : noncopyable {
public:
    using ChannelList = std::vector<Channel*>;

    explicit Poller(EventLoop* loop);
    virtual ~Poller();

    // 纯虚接口
    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;
    virtual void updateChannel(Channel* channel) = 0;
    virtual void removeChannel(Channel* channel) = 0;

    virtual bool hasChannel(Channel* channel) const;

    // 工厂方法
    static Poller* newDefaultPoller(EventLoop* loop);

    void assertInLoopThread() const;

protected:
    using ChannelMap = std::map<int, Channel*>;
    ChannelMap channels_;
    EventLoop* ownerLoop_;
};
}
```

### EpollPoller

```cpp
namespace csl {

class EpollPoller : public Poller {
public:
    explicit EpollPoller(EventLoop* loop);
    ~EpollPoller() override;

    Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;

private:
    static const int kInitEventListSize = 16;

    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
    void update(int operation, Channel* channel);

    int epollfd_;
    using EventList = std::vector<struct epoll_event>;
    EventList events_;
};
}
```

---

## 关键设计决策

| 决策点 | 选择 | 原因 |
|---|---|---|
| wakeup 机制 | eventfd（Linux only）| 本阶段只支持 Linux，eventfd 比 pipe 更简洁 |
| 互斥锁 | `std::mutex` | 仅用于保护 pendingFunctors_ 队列 |
| `TimerQueue` 相关 | 空骨架 | TimerQueue 在里程 #8 实现，现在预留接口但不实现 |
| `ThreadLocal` | `__thread` 或 `std::thread::id` | EventLoop 绑线程用 threadId 检查 |
| `Channel::tie()` | `std::weak_ptr<void>` | 与 muduo 一致，防止 handleEvent 中 owner 被析构 |

---

## 验收标准

- [ ] EventLoop::loop() 可正常启动和停止
- [ ] Channel 可注册读/写事件回调，handleEvent 正确分发
- [ ] Poller::poll() 正确返回活跃 Channel 列表
- [ ] EpollPoller 正确封装 epoll_create/epoll_ctl/epoll_wait
- [ ] queueInLoop 跨线程投递任务后 wakeup 唤醒 loop
- [ ] 单元测试 + 集成测试覆盖核心路径

---

## 步骤

| 步骤 | 内容 | 预估 |
|---|---|---|
| 3.1 | Channel 头文件 + 实现 | 1h |
| 3.2 | Poller + EpollPoller 头文件 + 实现 | 1.5h |
| 3.3 | EventLoop 头文件 + 实现 | 1.5h |
| 3.4 | 测试 + CMakeLists 更新 | 1h |
