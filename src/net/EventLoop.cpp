// Copyright 2026, cpp-server-lab
// EventLoop 实现
//
// 核心循环（loop 方法）：
//   1. poll(timeoutMs, &activeChannels_) → 获取活跃 Channel
//   2. 遍历 activeChannels_，调用 handleEvent() 分发事件
//   3. doPendingFunctors() → 执行跨线程投递的任务
//
// 唤醒机制（eventfd）：
//   - wakeup() 向 wakeupFd_ 写入 8 字节，触发 epoll 返回
//   - handleRead() 读取这 8 字节，排空 eventfd
//
// 线程安全：
//   - pendingFunctors_ 队列受 mutex_ 保护（跨线程写入）
//   - 其他成员仅在 IO 线程访问

#include "csl/net/EventLoop.h"
#include "csl/net/Channel.h"
#include "csl/net/Poller.h"
#include "csl/net/EpollPoller.h"
#include "csl/timer/TimerQueue.h"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <sstream>
#include <sys/eventfd.h>
#include <utility>
#include <unistd.h>

namespace csl {

// ---- 线程局部存储：当前线程的 EventLoop ----
namespace {
    __thread EventLoop* t_loopInThisThread = nullptr;

    // poll 默认超时：10 秒
    const int kPollTimeMs = 10000;
}

// ===== 构造 / 析构 =====

EventLoop::EventLoop()
    : looping_(false)
    , quit_(false)
    , eventHandling_(false)
    , callingPendingFunctors_(false)
    , threadId_(std::this_thread::get_id())
    , poller_(Poller::newDefaultPoller(this))
    , wakeupFd_(::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC))
    , wakeupChannel_(new Channel(this, wakeupFd_))
    , currentActiveChannel_(nullptr)
{
    if (wakeupFd_ < 0) {
        std::ostringstream oss;
        oss << "EventLoop::EventLoop() - eventfd() 失败: "
            << strerror(errno);
        perror(oss.str().c_str());
        std::abort();
    }

    // 断言：当前线程尚未绑定 EventLoop
    if (t_loopInThisThread) {
        std::abort(); // 每个线程仅允许一个 EventLoop
    }
    t_loopInThisThread = this;

    // 配置 wakeupChannel：监听读事件
    wakeupChannel_->setReadCallback(
        std::bind(&EventLoop::handleRead, this));
    wakeupChannel_->enableReading();

    // 初始化定时器队列
    timerQueue_ = std::make_unique<TimerQueue>(this);
}

EventLoop::~EventLoop() {
    // 确保 loop 已退出
    assert(!looping_);

    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

// ===== 循环控制 =====

void EventLoop::loop() {
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    quit_ = false;

    while (!quit_) {
        activeChannels_.clear();

        // 1. poll：等待 IO 事件
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);

        // 2. 分发活跃 Channel 的事件
        eventHandling_ = true;
        for (Channel* channel : activeChannels_) {
            currentActiveChannel_ = channel;
            channel->handleEvent(pollReturnTime_);
        }
        currentActiveChannel_ = nullptr;
        eventHandling_ = false;

        // 3. 执行跨线程投递的任务
        doPendingFunctors();
    }

    looping_ = false;
}

void EventLoop::quit() {
    quit_ = true;

    // 如果当前不在 IO 线程中，需要唤醒阻塞在 poll 中的 loop
    if (!isInLoopThread()) {
        wakeup();
    }
}

// ===== 任务投递 =====

void EventLoop::runInLoop(Functor cb) {
    if (isInLoopThread()) {
        cb();
    } else {
        queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(Functor cb) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingFunctors_.push_back(std::move(cb));
    }

    // 需要唤醒 loop 的情况：
    //   - 调用者不在 IO 线程（跨线程投递）
    //   - 正在执行 pendingFunctors_（需要重新 poll 后再处理新任务）
    if (!isInLoopThread() || callingPendingFunctors_) {
        wakeup();
    }
}

size_t EventLoop::queueSize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pendingFunctors_.size();
}

// ===== Channel 管理 =====

void EventLoop::updateChannel(Channel* channel) {
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel) {
    assert(channel->ownerLoop() == this);
    assertInLoopThread();

    // 安全性：不能在 handleEvent 中移除当前正在处理的 Channel
    if (eventHandling_) {
        assert(currentActiveChannel_ == channel
               || std::find(activeChannels_.begin(),
                            activeChannels_.end(),
                            channel) == activeChannels_.end());
    }

    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel) {
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    return poller_->hasChannel(channel);
}

// ===== 线程检查 =====

bool EventLoop::isInLoopThread() const {
    return threadId_ == std::this_thread::get_id();
}

void EventLoop::assertInLoopThread() {
    if (!isInLoopThread()) {
        abortNotInLoopThread();
    }
}

// ===== 唤醒 =====

void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n = ::write(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        // eventfd 写失败（极端情况）
        perror("EventLoop::wakeup() - write 失败");
    }
}

// ===== 静态方法 =====

EventLoop* EventLoop::getEventLoopOfCurrentThread() {
    return t_loopInThisThread;
}

// ===== 定时器 =====

TimerId EventLoop::runAt(Timestamp time, Timer::TimerCallback cb) {
    return timerQueue_->addTimer(std::move(cb), time, 0.0);
}

TimerId EventLoop::runAfter(double delay, Timer::TimerCallback cb) {
    Timestamp time = addTime(Timestamp::now(), delay);
    return runAt(time, std::move(cb));
}

TimerId EventLoop::runEvery(double interval, Timer::TimerCallback cb) {
    Timestamp time = addTime(Timestamp::now(), interval);
    return timerQueue_->addTimer(std::move(cb), time, interval);
}

void EventLoop::cancel(TimerId timerId) {
    timerQueue_->cancel(timerId);
}

// ===== 内部方法 =====

void EventLoop::handleRead() {
    uint64_t one = 0;
    ssize_t n = ::read(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        // 排空失败（不应该发生，除非 eventfd 被错误操作）
        std::ostringstream oss;
        oss << "EventLoop::handleRead() 读取 " << n
            << " 字节，期望 " << sizeof(one);
        perror(oss.str().c_str());
    }
}

void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        // 加锁交换，减少锁持有时间
        std::lock_guard<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (const Functor& func : functors) {
        func();
    }

    callingPendingFunctors_ = false;
}

void EventLoop::abortNotInLoopThread() {
    std::ostringstream oss;
    oss << "EventLoop::abortNotInLoopThread - EventLoop " << this
        << " 创建于 threadId=" << threadId_
        << "，当前 threadId=" << std::this_thread::get_id();
    perror(oss.str().c_str());
    std::abort();
}

}  // namespace csl
