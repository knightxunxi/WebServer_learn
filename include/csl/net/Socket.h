// Copyright 2026, cpp-server-lab
// Socket - RAII socket fd 封装
//
// 设计意图：
//   通过 RAII 管理 socket fd 的生命周期，提供 bind/listen/accept 等方法。
//   析构时自动关闭 fd，避免资源泄漏。
//
// 生命周期约束：
//   Socket 独占 fd，不支持拷贝，支持移动。

#pragma once

#include "csl/base/noncopyable.h"

namespace csl {

class InetAddress;

/// @brief RAII socket fd 封装
class Socket : noncopyable {
public:
    explicit Socket(int sockfd);
    ~Socket();

    int fd() const { return sockfd_; }

    // ---- 操作 ----
    void bindAddress(const InetAddress& localAddr);
    void listen();
    int accept(InetAddress* peerAddr);  // 返回新连接的 fd

    // ---- 属性设置 ----
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setTcpNoDelay(bool on);        // Nagle 算法开关
    void setKeepAlive(bool on);

    // ---- 关闭 ----
    void shutdownWrite();               // 半关闭（FIN）

private:
    const int sockfd_;
};

}  // namespace csl
