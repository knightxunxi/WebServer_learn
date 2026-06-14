// Copyright 2026, cpp-server-lab
// Poller 基类实现

#include "csl/net/Poller.h"
#include "csl/net/Channel.h"
#include "csl/net/EpollPoller.h"
#include "csl/net/EventLoop.h"

#include <cassert>

namespace csl {

Poller::Poller(EventLoop* loop)
    : ownerLoop_(loop)
{
}

Poller::~Poller() = default;

bool Poller::hasChannel(Channel* channel) const {
    assertInLoopThread();
    auto it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}

void Poller::assertInLoopThread() const {
    ownerLoop_->assertInLoopThread();
}

// 工厂方法：Linux 下默认返回 EpollPoller
Poller* Poller::newDefaultPoller(EventLoop* loop) {
    return new EpollPoller(loop);
}

}  // namespace csl
