// Copyright 2026, cpp-server-lab
// Acceptor - 连接接受器
//
// 设计意图：
//   封装 listen socket 和 accept 流程，将新连接通过回调分发给 TcpServer。
//   拥有 acceptChannel_（注册到 EventLoop），通过 enableReading 启动监听。
//
// 生命周期：
//   Acceptor 由 TcpServer 拥有，生命周期不跨线程。

#pragma once

#include "csl/base/noncopyable.h"
#include "csl/net/Socket.h"
#include "csl/net/Channel.h"

#include <functional>

namespace csl {

class EventLoop;
class InetAddress;

/// @brief 连接接受器
///
/// 在 EventLoop 中监听 accept socket，接收新连接并通过 NewConnectionCallback 分发。
class Acceptor : noncopyable {
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress& peerAddr)>;

    Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reusePort = true);
    ~Acceptor();

    // ---- 回调设置 ----
    void setNewConnectionCallback(NewConnectionCallback cb);

    // ---- 监听 ----
    void listen();

    // ---- 状态 ----
    bool listening() const { return listening_; }
    const InetAddress& localAddress() const { return listenAddr_; }

private:
    void handleRead();  // acceptChannel_ 的读回调

    EventLoop* loop_;
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listening_;
    int idleFd_;        // 用于处理 fd 耗尽时的 accept 错误
};

}  // namespace csl
