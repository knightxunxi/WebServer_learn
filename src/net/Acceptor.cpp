// Copyright 2026, cpp-server-lab
// Acceptor 实现
//
// 注意：Acceptor 在 EventLoop 线程中运行，所有操作均在 IO 线程。
//       handleRead 通过 Channel 回调执行，运行在 IO 线程上下文。

#include "csl/net/Acceptor.h"
#include "csl/net/EventLoop.h"
#include "csl/net/InetAddress.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <functional>
#include <sys/socket.h>
#include <utility>
#include <unistd.h>

namespace csl {

// ===== 构造 / 析构 =====

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reusePort)
    : loop_(loop)
    , listenAddr_(listenAddr)
    , acceptSocket_(::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP))
    , acceptChannel_(loop, acceptSocket_.fd())
    , listening_(false)
    , idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
    // 设置地址复用
    acceptSocket_.setReuseAddr(true);
    if (reusePort) {
        acceptSocket_.setReusePort(true);
    }

    // 绑定并监听
    acceptSocket_.bindAddress(listenAddr);
}

Acceptor::~Acceptor() {
    acceptChannel_.disableAll();
    acceptChannel_.remove();
    ::close(idleFd_);
}

// ===== 回调设置 =====

void Acceptor::setNewConnectionCallback(NewConnectionCallback cb) {
    newConnectionCallback_ = std::move(cb);
}

// ===== 监听 =====

void Acceptor::listen() {
    loop_->assertInLoopThread();
    listening_ = true;
    acceptSocket_.listen();

    // 注册读事件回调，准备 accept
    acceptChannel_.setReadCallback(
        std::bind(&Acceptor::handleRead, this));
    acceptChannel_.enableReading();
}

// ===== 内部方法 =====

void Acceptor::handleRead() {
    loop_->assertInLoopThread();

    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);

    if (connfd >= 0) {
        // accept 成功：通知上层
        if (newConnectionCallback_) {
            newConnectionCallback_(connfd, peerAddr);
        } else {
            ::close(connfd);
        }
    } else {
        // accept 失败
        if (errno == EMFILE) {
            // fd 耗尽：关闭 idleFd_ 释放一个 fd，然后重新打开
            // 这是 muduo 的经典防御策略
            ::close(idleFd_);
            idleFd_ = ::accept(acceptSocket_.fd(), nullptr, nullptr);
            ::close(idleFd_);
            idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
        // EAGAIN / EWOULDBLOCK / ECONNABORTED 等其他错误可忽略
    }
}

}  // namespace csl
