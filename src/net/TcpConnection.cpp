// Copyright 2026, cpp-server-lab
// TcpConnection 实现
//
// 当前阶段（里程 #4）：使用临时栈缓冲读取数据。
// 里程 #6 将引入 Buffer 类，替换为完整的 input/output Buffer。

#include "csl/net/TcpConnection.h"
#include "csl/net/EventLoop.h"
#include "csl/net/Socket.h"
#include "csl/net/Channel.h"

#include <cstring>
#include <unistd.h>

namespace csl {

// ===== 构造 / 析构 =====

TcpConnection::TcpConnection(EventLoop* loop,
                             const std::string& name,
                             int sockfd,
                             const InetAddress& localAddr,
                             const InetAddress& peerAddr)
    : loop_(loop)
    , name_(name)
    , state_(kConnecting)
    , socket_(new Socket(sockfd))
    , channel_(new Channel(loop, sockfd))
    , localAddr_(localAddr)
    , peerAddr_(peerAddr)
{
    // 配置 Channel 回调
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(
        std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(
        std::bind(&TcpConnection::handleError, this));

    // 基本 socket 配置
    socket_->setTcpNoDelay(true);
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection() {
    // 连接应该在 connectDestroyed() 中已被清理
}

// ===== 回调设置 =====

void TcpConnection::setConnectionCallback(const ConnectionCallback& cb) {
    connectionCallback_ = cb;
}

void TcpConnection::setMessageCallback(const MessageCallback& cb) {
    messageCallback_ = cb;
}

void TcpConnection::setWriteCompleteCallback(const WriteCompleteCallback& cb) {
    writeCompleteCallback_ = cb;
}

void TcpConnection::setCloseCallback(const CloseCallback& cb) {
    closeCallback_ = cb;
}

// ===== 连接管理 =====

void TcpConnection::connectEstablished() {
    loop_->assertInLoopThread();
    assert(state_ == kConnecting);
    setState(kConnected);

    // 将 TcpConnection 绑定到 Channel，防止 handleEvent 中析构
    channel_->tie(shared_from_this());
    channel_->enableReading();

    // 触发连接建立回调
    if (connectionCallback_) {
        connectionCallback_(shared_from_this());
    }
}

void TcpConnection::connectDestroyed() {
    loop_->assertInLoopThread();
    if (state_ == kConnected) {
        setState(kDisconnected);
        channel_->disableAll();
        if (connectionCallback_) {
            connectionCallback_(shared_from_this());
        }
    }
    channel_->remove();
}

// ===== 数据操作 =====

void TcpConnection::send(const std::string& message) {
    send(message.data(), message.size());
}

void TcpConnection::send(const void* data, size_t len) {
    if (state_ == kConnected) {
        if (loop_->isInLoopThread()) {
            sendInLoop(std::string(static_cast<const char*>(data), len));
        } else {
            loop_->runInLoop(
                std::bind(&TcpConnection::sendInLoop, this,
                          std::string(static_cast<const char*>(data), len)));
        }
    }
}

void TcpConnection::shutdown() {
    if (state_ == kConnected) {
        setState(kDisconnecting);
        loop_->runInLoop(
            std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

// ===== 上下文 =====

void TcpConnection::setContext(const std::any& context) {
    context_ = context;
}

const std::any& TcpConnection::getContext() const {
    return context_;
}

// ===== 内部事件处理 =====

void TcpConnection::handleRead(Timestamp receiveTime) {
    loop_->assertInLoopThread();

    // 临时栈缓冲读取（里程 #6 替换为 Buffer）
    char buf[65536];
    ssize_t n = ::read(channel_->fd(), buf, sizeof(buf));

    if (n > 0) {
        // 临时方案：存入 inputBuffer_，里程 #6 替换为 Buffer
        inputBuffer_.assign(buf, static_cast<size_t>(n));
        if (messageCallback_) {
            messageCallback_(shared_from_this(), receiveTime);
        }
    } else if (n == 0) {
        handleClose();
    } else {
        // n < 0
        if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            handleError();
        }
    }
}

void TcpConnection::handleWrite() {
    loop_->assertInLoopThread();

    if (channel_->isWriting()) {
        // 输出缓冲清空后，停用写事件监听
        channel_->disableWriting();

        if (writeCompleteCallback_) {
            loop_->queueInLoop(
                std::bind(writeCompleteCallback_, shared_from_this()));
        }

        // 如果处于正在关闭状态，执行关闭
        if (state_ == kDisconnecting) {
            shutdownInLoop();
        }
    }
}

void TcpConnection::handleClose() {
    loop_->assertInLoopThread();
    assert(state_ == kConnected || state_ == kDisconnecting);

    setState(kDisconnected);
    channel_->disableAll();

    // 触发关闭回调
    if (connectionCallback_) {
        connectionCallback_(shared_from_this());
    }

    if (closeCallback_) {
        closeCallback_(shared_from_this());
    }
}

void TcpConnection::handleError() {
    // 记录错误（生产环境应使用 Logger）
    // 关闭连接
    handleClose();
}

// ===== 内部 IO 线程方法 =====

void TcpConnection::sendInLoop(const std::string& message) {
    // 简化版：直接 write（里程 #6 引入 Buffer 后使用 output buffer）
    loop_->assertInLoopThread();
    if (state_ == kDisconnected) return;

    ssize_t n = ::write(channel_->fd(), message.data(), message.size());
    if (n < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            handleError();
        }
    }
}

void TcpConnection::shutdownInLoop() {
    loop_->assertInLoopThread();
    if (!channel_->isWriting()) {
        // 没有待写数据，直接关闭
        socket_->shutdownWrite();
    }
    // 如果还有待写数据，等 handleWrite 再关闭
}

}  // namespace csl
