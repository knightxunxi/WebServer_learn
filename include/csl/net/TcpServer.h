// Copyright 2026, cpp-server-lab
// TcpServer - TCP 服务器（支持多线程 IO 池）
//
// 设计意图：
//   封装 Acceptor 和 TcpConnection 集合，通过 EventLoopThreadPool
//   将连接分发到多个 IO 线程，实现 one loop per thread。
//   若 setThreadNum(0)，退化为单线程模式。
//
// 参考：muduo/net/TcpServer.h

#pragma once

#include "csl/base/noncopyable.h"
#include "csl/base/timestamp.h"
#include "csl/net/InetAddress.h"

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <string>

namespace csl {

class EventLoop;
class EventLoopThreadPool;
class Acceptor;
class Buffer;
class TcpConnection;

/// @brief TCP 服务器
class TcpServer : noncopyable {
public:
    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
    using ConnectionCallback  = std::function<void(const TcpConnectionPtr&)>;
    using MessageCallback     = std::function<void(const TcpConnectionPtr&, Buffer*, Timestamp)>;
    using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    TcpServer(EventLoop* loop, const InetAddress& listenAddr,
              const std::string& name);
    ~TcpServer();

    // ---- 配置 ----
    /// @brief 设置 IO 线程数（需在 start 前调用）
    void setThreadNum(int numThreads);

    // ---- 回调设置 ----
    void setConnectionCallback(const ConnectionCallback& cb);
    void setMessageCallback(const MessageCallback& cb);
    void setWriteCompleteCallback(const WriteCompleteCallback& cb);
    void setThreadInitCallback(const ThreadInitCallback& cb);

    // ---- 启动 ----
    void start();

    // ---- 状态 ----
    const std::string& name() const { return name_; }
    const std::string& ipPort() const { return ipPort_; }
    EventLoop* getLoop() const { return loop_; }

private:
    void newConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    EventLoop* loop_;
    const std::string ipPort_;
    const std::string name_;
    std::unique_ptr<Acceptor> acceptor_;
    std::unique_ptr<EventLoopThreadPool> threadPool_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    ThreadInitCallback threadInitCallback_;

    std::atomic<int> started_;
    int nextConnId_;
    using ConnectionMap = std::map<std::string, TcpConnectionPtr>;
    ConnectionMap connections_;
};

}  // namespace csl
