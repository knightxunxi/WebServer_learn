// Copyright 2026, cpp-server-lab
// Socket 实现

#include "csl/net/Socket.h"
#include "csl/net/InetAddress.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

namespace csl {

Socket::Socket(int sockfd)
    : sockfd_(sockfd) {}

Socket::~Socket() {
    ::close(sockfd_);
}

void Socket::bindAddress(const InetAddress& localAddr) {
    const struct sockaddr_in& addr = localAddr.getSockAddr();
    int ret = ::bind(sockfd_, reinterpret_cast<const struct sockaddr*>(&addr),
                     sizeof(addr));
    if (ret < 0) {
        // bind 失败是严重错误
        perror("Socket::bindAddress");
        std::abort();
    }
}

void Socket::listen() {
    int ret = ::listen(sockfd_, SOMAXCONN);
    if (ret < 0) {
        perror("Socket::listen");
        std::abort();
    }
}

int Socket::accept(InetAddress* peerAddr) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    socklen_t addrLen = sizeof(addr);

    int connfd = ::accept4(sockfd_,
                           reinterpret_cast<struct sockaddr*>(&addr),
                           &addrLen,
                           SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd >= 0) {
        peerAddr->setSockAddr(addr);
    }
    // 返回 -1 时由调用者处理（通常是 EAGAIN）
    return connfd;
}

// ---- 属性设置 ----

void Socket::setReuseAddr(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

void Socket::setReusePort(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}

void Socket::setTcpNoDelay(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}

void Socket::setKeepAlive(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}

void Socket::shutdownWrite() {
    ::shutdown(sockfd_, SHUT_WR);
}

}  // namespace csl
