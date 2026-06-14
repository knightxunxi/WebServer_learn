// Copyright 2026, cpp-server-lab
// EventLoopThread 实现

#include "csl/net/EventLoopThread.h"
#include "csl/net/EventLoop.h"

#include <cassert>

namespace csl {

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb,
                                 const std::string& name)
    : loop_(nullptr)
    , exiting_(false)
    , callback_(cb)
{
    (void)name;
}

EventLoopThread::~EventLoopThread() {
    exiting_ = true;
    if (loop_) {
        loop_->quit();
    }
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
}

EventLoop* EventLoopThread::startLoop() {
    assert(!thread_);
    assert(!loop_);

    thread_ = std::make_unique<std::thread>(
        &EventLoopThread::threadFunc, this);

    // 阻塞等待 loop_ 初始化完成
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] { return loop_ != nullptr; });
    }

    return loop_;
}

void EventLoopThread::threadFunc() {
    EventLoop loop;

    if (callback_) {
        callback_(&loop);
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop();

    // loop 退出后清理
    std::lock_guard<std::mutex> lock(mutex_);
    loop_ = nullptr;
}

}  // namespace csl
