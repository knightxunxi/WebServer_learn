// Copyright 2026, cpp-server-lab
// Timer - 定时器核心类
//
// 存储过期时间、回调和重复间隔。

#pragma once

#include "csl/base/timestamp.h"

#include <atomic>
#include <functional>
#include <utility>

namespace csl {

class Timer {
public:
    using TimerCallback = std::function<void()>;

    Timer(TimerCallback cb, Timestamp when, double interval)
        : callback_(std::move(cb))
        , expiration_(when)
        , interval_(interval)
        , repeat_(interval > 0.0)
        , sequence_(s_numCreated_.fetch_add(1) + 1)
    {
    }

    void run() const { callback_(); }

    Timestamp expiration() const { return expiration_; }
    bool repeat() const { return repeat_; }
    int64_t sequence() const { return sequence_; }

    // 如果可重复，计算下次超时时间
    void restart(Timestamp now);

    static int64_t numCreated() { return s_numCreated_.load(); }

private:
    const TimerCallback callback_;
    Timestamp expiration_;
    const double interval_;
    const bool repeat_;
    const int64_t sequence_;

    static std::atomic<int64_t> s_numCreated_;
};

}  // namespace csl
