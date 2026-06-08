// Copyright 2026, cpp-server-lab
// TimerQueue - 定时器管理器
//
// 设计意图：
//   使用 Linux timerfd 管理定时器，通过 Channel 注册到 EventLoop。
//   定时器按过期时间排序（std::set），支持一次性/重复定时器。
//
// 生命周期：
//   每个 EventLoop 最多一个 TimerQueue，由 EventLoop 独占管理。

#pragma once

#include "csl/base/noncopyable.h"
#include "csl/base/timestamp.h"
#include "csl/timer/Timer.h"
#include "csl/timer/TimerId.h"

#include <memory>
#include <set>
#include <utility>
#include <vector>

namespace csl {

class EventLoop;
class Channel;

class TimerQueue : noncopyable {
public:
    explicit TimerQueue(EventLoop* loop);
    ~TimerQueue();

    /// @brief 添加定时器
    /// @param cb   超时回调
    /// @param when 过期时间
    /// @param interval 重复间隔（> 0 表示重复定时器）
    TimerId addTimer(Timer::TimerCallback cb, Timestamp when, double interval);

    /// @brief 取消定时器
    void cancel(TimerId timerId);

private:
    // 定时器条目（过期时间 + 定时器指针）
    using Entry = std::pair<Timestamp, Timer*>;
    using TimerList = std::set<Entry>;

    void addTimerInLoop(Timer* timer);
    void cancelInLoop(TimerId timerId);

    // timerfd 可读时的回调
    void handleRead();

    // 获取所有已超时定时器
    std::vector<Entry> getExpired(Timestamp now);

    // 重置所有超时的重复定时器
    void reset(const std::vector<Entry>& expired, Timestamp now);

    // 将 timerfd 设置为最早超时时间
    void resetTimerfd(Timestamp expiration);

    // 将 timerfd 的时间设置函数
    void resetTimerfdFromNow(int timerfd, Timestamp expiration);

    /// @brief 插入定时器到有序集合，返回是否改变了最早超时时间
    bool insert(Timer* timer);

    EventLoop* loop_;
    const int timerfd_;
    std::unique_ptr<Channel> timerfdChannel_;
    TimerList timers_;              // 按过期时间排序的定时器

    // 用于 cancel 的计数器
    bool callingExpiredTimers_;
};

}  // namespace csl
