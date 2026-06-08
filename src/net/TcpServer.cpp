// Copyright 2026, cpp-server-lab
// TcpServer 实现（多线程版）

#include "csl/net/TcpServer.h"
#include "csl/net/Acceptor.h"
#include "csl/net/EventLoop.h"
#include "csl/net/EventLoopThreadPool.h"
#include "csl/net/TcpConnection.h"

#include <cassert>

namespace csl {

TcpServer::TcpServer(EventLoop* loop,
                     const InetAddress& listenAddr,
                     const std::string& name)
    : loop_(loop)
    , ipPort_(listenAddr.toIpPort())
    , name_(name)
    , acceptor_(new Acceptor(loop, listenAddr))
    , threadPool_(new EventLoopThreadPool(loop, name_))
    , started_(0)
    , nextConnId_(1)
{
    acceptor_->setNewConnectionCallback(
        std::bind(&TcpServer::newConnection, this,
                  std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer() {
    loop_->assertInLoopThread();
}

// ===== 配置 =====

void TcpServer::setThreadNum(int numThreads) {
    assert(numThreads >= 0);
    threadPool_->setThreadNum(numThreads);
}

// ===== 回调设置 =====

void TcpServer::setConnectionCallback(const ConnectionCallback& cb) {
    connectionCallback_ = cb;
}

void TcpServer::setMessageCallback(const MessageCallback& cb) {
    messageCallback_ = cb;
}

void TcpServer::setWriteCompleteCallback(const WriteCompleteCallback& cb) {
    writeCompleteCallback_ = cb;
}

void TcpServer::setThreadInitCallback(const ThreadInitCallback& cb) {
    threadInitCallback_ = cb;
}

// ===== 启动 =====

void TcpServer::start() {
    if (started_.exchange(1) == 0) {
        threadPool_->start(threadInitCallback_);
        acceptor_->listen();
    }
}

// ===== 新连接处理 =====

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr) {
    loop_->assertInLoopThread();

    // 选择一个 IO 线程（round-robin）
    EventLoop* ioLoop = threadPool_->getNextLoop();

    // 生成连接名
    char buf[64];
    snprintf(buf, sizeof(buf), "%s-%s#%d",
             ipPort_.c_str(), name_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = buf;

    // 构造 TcpConnection
    InetAddress localAddr(acceptor_->localAddress());
    TcpConnectionPtr conn = std::make_shared<TcpConnection>(
        ioLoop, connName, sockfd, localAddr, peerAddr);

    // 加入连接表
    connections_[connName] = conn;

    // 设置回调
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));

    // 在 IO 线程中建立连接
    ioLoop->runInLoop(
        std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn) {
    loop_->runInLoop(
        std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn) {
    loop_->assertInLoopThread();
    size_t n = connections_.erase(conn->name());
    assert(n == 1); (void)n;

    // 在 conn 所属的 IO 线程执行 connectDestroyed
    conn->getLoop()->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed, conn));
}

}  // namespace csl
