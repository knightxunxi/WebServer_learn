// Copyright 2026, cpp-server-lab
// EventLoopThreadPool 实现

#include "csl/net/EventLoopThreadPool.h"
#include "csl/net/EventLoop.h"
#include "csl/net/EventLoopThread.h"

#include <cassert>

namespace csl {

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop,
                                         const std::string& name)
    : baseLoop_(baseLoop)
    , name_(name)
    , started_(false)
    , numThreads_(0)
    , next_(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool() = default;

void EventLoopThreadPool::start(const ThreadInitCallback& cb) {
    assert(!started_);
    baseLoop_->assertInLoopThread();
    started_ = true;

    for (int i = 0; i < numThreads_; ++i) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%s%d", name_.c_str(), i);
        auto* t = new EventLoopThread(cb, buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        loops_.push_back(t->startLoop());
    }

    // 不启动工作线程时，getNextLoop 返回 baseLoop
    if (numThreads_ == 0 && cb) {
        cb(baseLoop_);
    }
}

EventLoop* EventLoopThreadPool::getNextLoop() {
    baseLoop_->assertInLoopThread();
    assert(started_);

    EventLoop* loop = baseLoop_;  // 默认返回 baseLoop
    if (!loops_.empty()) {
        // round-robin
        loop = loops_[next_];
        ++next_;
        if (static_cast<size_t>(next_) >= loops_.size()) {
            next_ = 0;
        }
    }
    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops() {
    baseLoop_->assertInLoopThread();
    assert(started_);

    if (loops_.empty()) {
        return { baseLoop_ };
    }
    return loops_;
}

}  // namespace csl
