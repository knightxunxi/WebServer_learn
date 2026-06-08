// Copyright 2026, cpp-server-lab
// TcpConnection - TCP 连接代表
//
// 设计意图：
//   管理一个已建立的 TCP 连接的生命周期：读写、关闭、错误处理。
//   每个 TcpConnection 绑定一个 Channel（读写事件）和 input/output Buffer。
//   通过 shared_ptr 管理生命周期，确保回调执行期间对象不被析构。
//
// 生命周期约束：
//   - TcpConnection 由 TcpServer 用 shared_ptr 管理
//   - 在所属 EventLoop 线程中创建和销毁
//   - 通过 tie() 绑定到 Channel，防止 handleEvent 中析构
//
// 参考：muduo/net/TcpConnection.h

#pragma once

#include "csl/base/noncopyable.h"
#include "csl/net/InetAddress.h"
#include "csl/net/Buffer.h"
#include "csl/base/timestamp.h"

#include <any>
#include <functional>
#include <memory>
#include <string>

namespace csl {

class EventLoop;
class Socket;
class Channel;

/// @brief TCP 连接
///
/// 使用 shared_from_this 机制，通过 Channel::tie() 确保回调安全。
/// 提供连接建立/消息到达/写完成/关闭/高水位等回调。
class TcpConnection
    : noncopyable
    , public std::enable_shared_from_this<TcpConnection>
{
public:
    // ---- 回调类型 ----
    using ConnectionCallback = std::function<void(const std::shared_ptr<TcpConnection>&)>;
    using MessageCallback    = std::function<void(const std::shared_ptr<TcpConnection>&, Buffer*, Timestamp)>;
    using WriteCompleteCallback = std::function<void(const std::shared_ptr<TcpConnection>&)>;
    using CloseCallback      = std::function<void(const std::shared_ptr<TcpConnection>&)>;

    /// @brief 构造一个 TCP 连接
    /// @param loop 所属 EventLoop
    /// @param name 连接名（用于日志）
    /// @param sockfd 已 accept 的 socket fd
    /// @param localAddr 本地地址
    /// @param peerAddr  对端地址
    TcpConnection(EventLoop* loop,
                  const std::string& name,
                  int sockfd,
                  const InetAddress& localAddr,
                  const InetAddress& peerAddr);
    ~TcpConnection();

    // ---- getter ----
    EventLoop* getLoop() const { return loop_; }
    const std::string& name() const { return name_; }
    const InetAddress& localAddress() const { return localAddr_; }
    const InetAddress& peerAddress() const { return peerAddr_; }
    bool connected() const { return state_ == kConnected; }

    // ---- 回调设置 ----
    void setConnectionCallback(const ConnectionCallback& cb);
    void setMessageCallback(const MessageCallback& cb);
    void setWriteCompleteCallback(const WriteCompleteCallback& cb);
    void setCloseCallback(const CloseCallback& cb);

    // ---- 连接管理（由 TcpServer 调用） ----
    void connectEstablished();   // 连接建立后调用（注册读写事件）
    void connectDestroyed();     // 连接销毁前调用（清理 Channel）

    // ---- 数据操作 ----
    /// @brief 发送数据（线程安全，可在任意线程调用）
    void send(const std::string& message);
    void send(const void* data, size_t len);

    /// @brief 获取输入缓冲，仅用于少量测试或调试；业务回调优先使用 MessageCallback 参数
    Buffer* inputBuffer() { return &inputBuffer_; }

    /// @brief 关闭连接（线程安全）
    void shutdown();

    // ---- 连接上下文 ----
    void setContext(const std::any& context);
    const std::any& getContext() const;

private:
    enum StateE { kConnecting, kConnected, kDisconnecting, kDisconnected };

    void setState(StateE s) { state_ = s; }

    // 读/写/关闭/错误事件处理
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    // 在 IO 线程发送数据
    void sendInLoop(const std::string& message);
    void shutdownInLoop();

    EventLoop* loop_;
    const std::string name_;
    StateE state_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    CloseCallback closeCallback_;

    std::any context_;

    Buffer inputBuffer_;
    Buffer outputBuffer_;
};

}  // namespace csl
