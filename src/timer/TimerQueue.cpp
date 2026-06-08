// Copyright 2026, cpp-server-lab
// TimerQueue 实现
//
// 平台依赖：timerfd_create / timerfd_settime (Linux 2.6.25+)

#include "csl/timer/TimerQueue.h"
#include "csl/net/EventLoop.h"
#include "csl/net/Channel.h"

#include <cstring>
#include <sys/timerfd.h>
#include <unistd.h>
#include <atomic>

namespace csl {

// 静态计数器初始化
std::atomic<int64_t> Timer::s_numCreated_(0);

// ===== Timer::restart =====

void Timer::restart(Timestamp now) {
    if (repeat_) {
        // 从当前时间计算下一次超时，避免累计误差
        expiration_ = addTime(now, interval_);
    } else {
        expiration_ = Timestamp::invalid();
    }
}

// ===== 辅助：timerfd 操作 =====

static int createTimerfd() {
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
                                   TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerfd < 0) {
        perror("TimerQueue: timerfd_create 失败");
        std::abort();
    }
    return timerfd;
}

static struct timespec howMuchTimeFromNow(Timestamp when) {
    int64_t microseconds = when.microSecondsSinceEpoch()
                         - Timestamp::now().microSecondsSinceEpoch();
    if (microseconds < 100) {
        microseconds = 100;  // 最小 100us，避免频繁唤醒
    }
    struct timespec ts;
    ts.tv_sec  = static_cast<time_t>(microseconds / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>((microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
    return ts;
}

static void resetTimerfd(int timerfd, Timestamp expiration) {
    struct itimerspec newValue;
    struct itimerspec oldValue;
    memset(&newValue, 0, sizeof(newValue));
    memset(&oldValue, 0, sizeof(oldValue));

    newValue.it_value = howMuchTimeFromNow(expiration);
    int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
    if (ret) {
        perror("TimerQueue: timerfd_settime 失败");
    }
}

static void readTimerfd(int timerfd) {
    uint64_t howmany;
    ssize_t n = ::read(timerfd, &howmany, sizeof(howmany));
    if (n != sizeof(howmany)) {
        perror("TimerQueue: read timerfd 失败");
    }
}

// ===== TimerQueue =====

TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop)
    , timerfd_(createTimerfd())
    , timerfdChannel_(new Channel(loop, timerfd_))
    , callingExpiredTimers_(false)
{
    timerfdChannel_->setReadCallback(
        std::bind(&TimerQueue::handleRead, this));
    timerfdChannel_->enableReading();
}

TimerQueue::~TimerQueue() {
    timerfdChannel_->disableAll();
    timerfdChannel_->remove();
    ::close(timerfd_);

    // 清理未执行的定时器
    for (const Entry& entry : timers_) {
        delete entry.second;
    }
}

TimerId TimerQueue::addTimer(Timer::TimerCallback cb,
                              Timestamp when,
                              double interval) {
    Timer* timer = new Timer(std::move(cb), when, interval);
    loop_->runInLoop(
        std::bind(&TimerQueue::addTimerInLoop, this, timer));
    return TimerId(timer, timer->sequence());
}

void TimerQueue::cancel(TimerId timerId) {
    loop_->runInLoop(
        std::bind(&TimerQueue::cancelInLoop, this, timerId));
}

// ===== 内部方法 =====

void TimerQueue::addTimerInLoop(Timer* timer) {
    loop_->assertInLoopThread();

    bool earliestChanged = insert(timer);
    if (earliestChanged) {
        // 新定时器是最早的，需要重置 timerfd
        resetTimerfd(timerfd_, timer->expiration());
    }
}

bool TimerQueue::insert(Timer* timer) {
    loop_->assertInLoopThread();
    bool earliestChanged = false;
    Timestamp when = timer->expiration();

    auto it = timers_.begin();
    if (it == timers_.end() || when < it->first) {
        earliestChanged = true;
    }
    timers_.insert(Entry(when, timer));
    return earliestChanged;
}

void TimerQueue::cancelInLoop(TimerId timerId) {
    loop_->assertInLoopThread();

    auto it = timers_.find(Entry(timerId.timer_->expiration(),
                                 timerId.timer_));
    if (it != timers_.end()) {
        delete it->second;
        timers_.erase(it);
    }
}

void TimerQueue::handleRead() {
    loop_->assertInLoopThread();

    Timestamp now = Timestamp::now();
    readTimerfd(timerfd_);

    std::vector<Entry> expired = getExpired(now);

    callingExpiredTimers_ = true;
    for (const Entry& entry : expired) {
        entry.second->run();
    }
    callingExpiredTimers_ = false;

    reset(expired, now);
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now) {
    std::vector<Entry> expired;
    Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));

    // 找到第一个过期时间 > now 的位置
    auto end = timers_.lower_bound(sentry);
    assert(end == timers_.end() || now < end->first);

    // 将 [begin, end) 范围内的定时器移出
    std::copy(timers_.begin(), end, std::back_inserter(expired));
    timers_.erase(timers_.begin(), end);

    return expired;
}

void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now) {
    for (const Entry& entry : expired) {
        Timer* timer = entry.second;
        if (timer->repeat()) {
            timer->restart(now);
            insert(timer);
        } else {
            delete timer;
        }
    }

    // 重设定时器到下一个最早过期时间
    if (!timers_.empty()) {
        Timestamp nextExpire = timers_.begin()->second->expiration();
        if (nextExpire.valid()) {
            resetTimerfd(timerfd_, nextExpire);
        }
    }
}

}  // namespace csl
