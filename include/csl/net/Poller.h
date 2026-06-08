// Copyright 2026, cpp-server-lab
// Poller - IO 多路复用抽象基类
//
// 设计意图：
//   Poller 定义 IO 多路复用的统一接口，具体实现在 EpollPoller 中。
//   未来可扩展 PollPoller（实验对比）而无需修改 EventLoop。
//
// 参考：muduo/net/Poller.h

#pragma once

#include "csl/base/noncopyable.h"
#include "csl/base/timestamp.h"

#include <map>
#include <vector>

namespace csl {

class Channel;
class EventLoop;

/// @brief IO 多路复用抽象基类
///
/// 管理 fd → Channel* 的映射，提供 poll/updateChannel/removeChannel 纯虚接口。
/// 子类实现具体的系统调用（epoll、poll 等）。
class Poller : noncopyable {
public:
    using ChannelList = std::vector<Channel*>;

    explicit Poller(EventLoop* loop);
    virtual ~Poller();

    /// @brief 轮询 IO 事件，阻塞直到超时或有事件到达
    /// @param timeoutMs     超时毫秒数
    /// @param activeChannels 输出参数：填充活跃 Channel 列表
    /// @return poll 返回时的 Timestamp
    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;

    /// @brief 更新 Channel 监听的事件（ADD 或 MOD）
    virtual void updateChannel(Channel* channel) = 0;

    /// @brief 移除 Channel 的监听（DEL）
    virtual void removeChannel(Channel* channel) = 0;

    /// @brief 检查 Channel 是否在管理列表中
    virtual bool hasChannel(Channel* channel) const;

    /// @brief 工厂方法：创建当前平台的默认 Poller
    ///        Linux 下返回 EpollPoller
    static Poller* newDefaultPoller(EventLoop* loop);

    /// @brief 断言调用线程是 EventLoop 线程
    void assertInLoopThread() const;

protected:
    /// fd → Channel* 映射，子类可访问
    using ChannelMap = std::map<int, Channel*>;
    ChannelMap channels_;

    EventLoop* ownerLoop_;
};

}  // namespace csl
