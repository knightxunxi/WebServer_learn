// Copyright 2026, cpp-server-lab
// TcpConnection 实现
//
// TcpConnection 负责 TCP 字节流的读写缓冲和连接生命周期。
// 读路径使用 inputBuffer_ 累积数据，业务回调从 Buffer 中消费；
// 写路径优先直接 write，未写完的数据进入 outputBuffer_ 并注册写事件。

#include "csl/net/TcpConnection.h"
#include "csl/net/EventLoop.h"
#include "csl/net/Socket.h"
#include "csl/net/Channel.h"

#include <cassert>
#include <cerrno>
#include <cstring>
#include <functional>
#include <utility>
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
    if (state_ != kDisconnected) {
        setState(kDisconnected);
        channel_->disableAll();
        if (connectionCallback_) {
            connectionCallback_(shared_from_this());
        }
    }
    if (!channel_->isNoneEvent()) {
        channel_->disableAll();
    }
    channel_->remove();
}

// ===== 数据操作 =====

void TcpConnection::send(const std::string& message) {
    send(message.data(), message.size());
}

void TcpConnection::send(const void* data, size_t len) {
    if (state_ == kConnected) {
        std::string message(static_cast<const char*>(data), len);
        if (loop_->isInLoopThread()) {
            sendInLoop(message);
        } else {
            auto self = shared_from_this();
            loop_->runInLoop(
                [self, message = std::move(message)]() {
                    self->sendInLoop(message);
                });
        }
    }
}

void TcpConnection::shutdown() {
    auto self = shared_from_this();
    loop_->runInLoop([self]() {
        if (self->state_ == kConnected) {
            self->setState(kDisconnecting);
            self->shutdownInLoop();
        }
    });
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

    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);

    if (n > 0) {
        if (messageCallback_) {
            messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
        }
    } else if (n == 0) {
        handleClose();
    } else {
        // n < 0
        if (savedErrno != EAGAIN && savedErrno != EWOULDBLOCK && savedErrno != EINTR) {
            errno = savedErrno;
            handleError();
        }
    }
}

void TcpConnection::handleWrite() {
    loop_->assertInLoopThread();

    if (channel_->isWriting()) {
        ssize_t n = ::write(channel_->fd(),
                            outputBuffer_.peek(),
                            outputBuffer_.readableBytes());
        if (n > 0) {
            outputBuffer_.retrieve(static_cast<size_t>(n));
            if (outputBuffer_.readableBytes() == 0) {
                channel_->disableWriting();

                if (writeCompleteCallback_) {
                    loop_->queueInLoop(
                        std::bind(writeCompleteCallback_, shared_from_this()));
                }

                if (state_ == kDisconnecting) {
                    shutdownInLoop();
                }
            }
        } else if (n < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                handleError();
            }
        }
    } else {
        // 理论上不会发生，保留防御性处理，避免写事件状态异常后反复触发。
        channel_->disableWriting();
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
    loop_->assertInLoopThread();
    if (state_ == kDisconnected) return;

    size_t remaining = message.size();
    ssize_t nwrote = 0;
    bool faultError = false;

    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
        nwrote = ::write(channel_->fd(), message.data(), message.size());
        if (nwrote >= 0) {
            remaining = message.size() - static_cast<size_t>(nwrote);
            if (remaining == 0 && writeCompleteCallback_) {
                loop_->queueInLoop(
                    std::bind(writeCompleteCallback_, shared_from_this()));
            }
        } else {
            nwrote = 0;
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                faultError = true;
                handleError();
            }
        }
    }

    if (!faultError && remaining > 0) {
        outputBuffer_.append(message.data() + nwrote, remaining);
        if (!channel_->isWriting()) {
            channel_->enableWriting();
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
