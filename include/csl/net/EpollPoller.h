// Copyright 2026, cpp-server-lab
// EpollPoller - Linux epoll 实现
//
// 设计意图：
//   封装 epoll_create/epoll_ctl/epoll_wait 系统调用，
//   将 epoll_event 转换为 Channel 的 revents 并分发给 EventLoop。
//
// 参考：muduo/net/poller/EPollPoller.h

#pragma once

#include "csl/net/Poller.h"

#include <vector>
#include <sys/epoll.h>

namespace csl {

/// @brief Linux epoll(7) 实现的 IO 多路复用
///
/// 拥有 epoll fd，负责 epoll_ctl(ADD/MOD/DEL) 和 epoll_wait。
/// 默认使用 LT 触发模式（后续可扩展 ET）。
class EpollPoller : public Poller {
public:
    explicit EpollPoller(EventLoop* loop);
    ~EpollPoller() override;

    Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;

private:
    // epoll_wait 事件数组初始大小
    static const int kInitEventListSize = 16;

    /// 将 epoll_wait 返回的事件数组填充到活跃 Channel 列表
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;

    /// 统一的 epoll_ctl 封装
    /// @param operation EPOLL_CTL_ADD / MOD / DEL
    void update(int operation, Channel* channel);

    int epollfd_;
    using EventList = std::vector<struct epoll_event>;
    EventList events_;          // epoll_wait 用的缓冲区，动态扩容
};

}  // namespace csl
