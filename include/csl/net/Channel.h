// Copyright 2026, cpp-server-lab
// Channel - IO 通道封装，不拥有 fd
//
// 设计意图：
//   将 fd 与其事件回调绑定，通过 EventLoop 管理事件监听。
//   Channel 不拥有 fd —— fd 的生命周期由 Socket/Acceptor/TcpConnection 管理。
//
// 生命周期约束：
//   - Channel 必须在所属 EventLoop 的线程中析构（或确保不在 handleEvent 中）
//   - handleEvent 期间禁止销毁 Channel（通过 tie() 机制防止）
//
// 参考：muduo/net/Channel.h

#pragma once

#include "csl/base/noncopyable.h"
#include "csl/base/timestamp.h"

#include <functional>
#include <memory>

#include <sys/epoll.h>

namespace csl {

class EventLoop;

/// @brief IO 通道，封装 fd 及其事件回调
///
/// 每个 Channel 绑定到一个 EventLoop，可注册读/写/关闭/错误回调。
/// enableReading/enableWriting 会自动通知 EventLoop 更新 epoll 监听。
class Channel : noncopyable {
public:
    // ---- 事件常量 ----
    static const int kNoneEvent  = 0;
    static const int kReadEvent  = EPOLLIN | EPOLLPRI;
    static const int kWriteEvent = EPOLLOUT;

    // ---- 回调类型 ----
    /// 读回调：携带 receiveTime 参数，方便统计延迟
    using ReadEventCallback = std::function<void(Timestamp)>;
    /// 通用事件回调（写/关闭/错误）
    using EventCallback = std::function<void()>;

    /// @brief 构造 Channel，绑定到 EventLoop
    /// @param loop 所属 EventLoop
    /// @param fd   要监控的文件描述符
    Channel(EventLoop* loop, int fd);
    ~Channel();

    // ---- 回调设置 ----
    void setReadCallback(ReadEventCallback cb);
    void setWriteCallback(EventCallback cb);
    void setCloseCallback(EventCallback cb);
    void setErrorCallback(EventCallback cb);

    // ---- 事件控制 ----
    /// 启用读事件监听（自动 update）
    void enableReading();
    /// 停用读事件监听（自动 update）
    void disableReading();
    /// 启用写事件监听（自动 update）
    void enableWriting();
    /// 停用写事件监听（自动 update）
    void disableWriting();
    /// 停用所有事件监听（自动 update）
    void disableAll();

    // ---- 状态查询 ----
    int fd() const { return fd_; }
    int events() const { return events_; }
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isReading() const { return events_ & kReadEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }

    // ---- Poller 接口（仅供 Poller 调用） ----
    int index() const { return index_; }
    void set_index(int idx) { index_ = idx; }
    void set_revents(int revt) { revents_ = revt; }
    int revents() const { return revents_; }

    /// @brief 事件分发（由 EventLoop::loop 每轮 poll 后调用）
    void handleEvent(Timestamp receiveTime);

    /// @brief 从所属 EventLoop 中移除自身
    void remove();

    EventLoop* ownerLoop() const { return loop_; }

    /// @brief 绑定 owner 对象，防止 handleEvent 中 owner 被销毁
    ///
    /// handleEvent 前会尝试 lock() 该 weak_ptr：
    ///   - 若 lock() 成功 → 正常执行回调
    ///   - 若 lock() 失败（owner 已销毁） → 跳过回调
    void tie(const std::shared_ptr<void>& obj);

private:
    /// 通知 EventLoop 更新此 Channel 的监听事件
    void update();

    // ---- 事件处理子函数 ----
    void handleEventWithGuard(Timestamp receiveTime);

    EventLoop* loop_;
    const int fd_;
    int events_;                // 关心的事件位掩码
    int revents_;               // Poller 返回的已就绪事件
    int index_;                 // Poller 内部使用的索引（-1 = 未注册）

    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;

    // tie 机制：弱引用 owner，防止 handleEvent 中 owner 析构
    std::weak_ptr<void> tie_;
    bool tied_;
    bool eventHandling_;        // 是否正在执行 handleEvent
    bool addedToLoop_;          // 是否已添加到 EventLoop
};

}  // namespace csl
