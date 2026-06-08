// Copyright 2026, cpp-server-lab
// Channel 实现
//
// 注意：update() 会调用 EventLoop::updateChannel()，间接进入 Poller。
//       handleEvent 期间禁止修改事件注册。

#include "csl/net/Channel.h"
#include "csl/net/EventLoop.h"

#include <cassert>
#include <sstream>

namespace csl {

// ===== 构造 / 析构 =====

Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop)
    , fd_(fd)
    , events_(0)
    , revents_(0)
    , index_(-1)
    , tied_(false)
    , eventHandling_(false)
    , addedToLoop_(false)
{
}

Channel::~Channel() {
    // 安全断言：析构时不在事件处理中
    assert(!eventHandling_);
    assert(!addedToLoop_);
}

// ===== 回调设置 =====

void Channel::setReadCallback(ReadEventCallback cb) {
    readCallback_ = std::move(cb);
}

void Channel::setWriteCallback(EventCallback cb) {
    writeCallback_ = std::move(cb);
}

void Channel::setCloseCallback(EventCallback cb) {
    closeCallback_ = std::move(cb);
}

void Channel::setErrorCallback(EventCallback cb) {
    errorCallback_ = std::move(cb);
}

// ===== 事件控制 =====

void Channel::enableReading() {
    events_ |= kReadEvent;
    update();
}

void Channel::disableReading() {
    events_ &= ~kReadEvent;
    update();
}

void Channel::enableWriting() {
    events_ |= kWriteEvent;
    update();
}

void Channel::disableWriting() {
    events_ &= ~kWriteEvent;
    update();
}

void Channel::disableAll() {
    events_ = kNoneEvent;
    update();
}

void Channel::remove() {
    assert(isNoneEvent());
    addedToLoop_ = false;
    loop_->removeChannel(this);
}

// ===== 事件分发 =====

void Channel::handleEvent(Timestamp receiveTime) {
    eventHandling_ = true;

    // tie 保护：如果 owner 已销毁，跳过回调
    std::shared_ptr<void> guard;
    if (tied_) {
        guard = tie_.lock();
        if (!guard) {
            eventHandling_ = false;
            return;
        }
    }

    // 处理关闭事件（优先级最高，避免后续操作已关闭的 fd）
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        if (closeCallback_) closeCallback_();
    }

    // 处理错误事件
    if (revents_ & EPOLLERR) {
        if (errorCallback_) errorCallback_();
    }

    // 处理读事件
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        if (readCallback_) readCallback_(receiveTime);
    }

    // 处理写事件
    if (revents_ & EPOLLOUT) {
        if (writeCallback_) writeCallback_();
    }

    eventHandling_ = false;
}

// ===== 生命周期绑定 =====

void Channel::tie(const std::shared_ptr<void>& obj) {
    tie_ = obj;
    tied_ = true;
}

// ===== 内部方法 =====

void Channel::update() {
    addedToLoop_ = true;
    loop_->updateChannel(this);
}

}  // namespace csl
