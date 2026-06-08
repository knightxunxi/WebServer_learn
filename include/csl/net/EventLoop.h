// Copyright 2026, cpp-server-lab
// EventLoop - 单线程事件循环
//
// 设计意图：
//   one loop per thread 的核心，每个线程最多一个 EventLoop。
//   封装 Poller、Channel 管理、任务队列和唤醒机制。
//
// 生命周期约束：
//   - EventLoop 不属于任何线程，但 loop() 必须在创建它的线程中调用
//   - 析构前必须确保 loop() 已退出
//   - 非线程安全的成员只在 IO 线程访问（pendingFunctors_ 除外）
//
// 参考：muduo/net/EventLoop.h

#pragma once

#include "csl/base/noncopyable.h"
#include "csl/base/timestamp.h"
#include "csl/timer/TimerId.h"
#include "csl/timer/Timer.h"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace csl {

class Channel;
class Poller;
class TimerQueue;

/// @brief Reactor 事件循环
///
/// 每个线程最多一个 EventLoop。核心循环：
///   1. poll() 等待 IO 事件
///   2. 分发活跃 Channel 的回调
///   3. 执行跨线程投递的 pending functors
class EventLoop : noncopyable {
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // ---- 循环控制 ----

    /// @brief 启动事件循环（阻塞直到 quit() 被调用）
    /// @pre 必须在创建 EventLoop 的线程中调用
    void loop();

    /// @brief 退出事件循环
    ///
    /// 线程安全性：可在任意线程调用。
    /// 如果当前正在 poll() 阻塞中，会自动唤醒。
    void quit();

    // ---- 任务投递 ----

    /// @brief 在 IO 线程执行回调（立即或排队）
    /// 如果调用者已在 IO 线程 → 立即执行
    /// 否则 → queueInLoop + wakeup
    void runInLoop(Functor cb);

    /// @brief 将回调加入队列，在下次 poll 返回后执行
    /// 可在任意线程调用
    void queueInLoop(Functor cb);

    /// @brief 待处理回调队列大小
    size_t queueSize() const;

    // ---- Channel 管理 ----

    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    // ---- 线程检查 ----

    /// @brief 当前线程是否是 IO 线程
    bool isInLoopThread() const;

    /// @brief 断言当前在 IO 线程（否则 abort）
    void assertInLoopThread();

    // ---- 唤醒 ----

    /// @brief 唤醒阻塞在 poll() 的循环（通过 eventfd）
    void wakeup();

    // ---- 定时器 ----

    /// @brief 在指定时刻执行回调
    TimerId runAt(Timestamp time, Timer::TimerCallback cb);

    /// @brief 在延迟 delay 秒后执行回调
    TimerId runAfter(double delay, Timer::TimerCallback cb);

    /// @brief 每隔 interval 秒重复执行回调
    TimerId runEvery(double interval, Timer::TimerCallback cb);

    /// @brief 取消定时器
    void cancel(TimerId timerId);

    // ---- 静态方法 ----

    /// @brief 返回当前线程的 EventLoop 实例（可能为 nullptr）
    static EventLoop* getEventLoopOfCurrentThread();

private:
    // ---- 内部方法 ----
    void handleRead();               // wakeupChannel 的读回调
    void doPendingFunctors();        // 执行待处理回调队列
    void abortNotInLoopThread();     // 非 IO 线程调用时终止

    // ---- 状态标志 ----
    bool looping_;
    std::atomic<bool> quit_;
    bool eventHandling_;             // 是否正在分发活跃 Channel
    bool callingPendingFunctors_;    // 是否正在执行 pendingFunctors_

    const std::thread::id threadId_;
    Timestamp pollReturnTime_;

    // ---- Poller ----
    std::unique_ptr<Poller> poller_;

    // ---- 唤醒机制（eventfd） ----
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;

    // ---- 活跃 Channel ----
    using ChannelList = std::vector<Channel*>;
    ChannelList activeChannels_;
    Channel* currentActiveChannel_;

    // ---- 待处理任务队列 ----
    mutable std::mutex mutex_;
    std::vector<Functor> pendingFunctors_;

    // ---- 定时器 ----
    std::unique_ptr<TimerQueue> timerQueue_;
};

}  // namespace csl
