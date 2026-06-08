// Copyright 2026, cpp-server-lab
// TimerId - 定时器轻量句柄，用于取消定时器

#pragma once

#include <cstdint>

namespace csl {

class Timer;

class TimerId {
public:
    TimerId() : timer_(nullptr), sequence_(0) {}
    TimerId(Timer* timer, int64_t seq)
        : timer_(timer), sequence_(seq) {}

    friend class TimerQueue;

private:
    Timer* timer_;
    int64_t sequence_;
};

}  // namespace csl
