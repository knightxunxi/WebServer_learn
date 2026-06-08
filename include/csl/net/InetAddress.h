// Copyright 2026, cpp-server-lab
// InetAddress - IPv4 地址与端口封装
//
// 设计意图：
//   统一 sockaddr_in 的构造和转换，隐藏结构体细节。
//   提供字符串解析（"ip:port" / "port"）和格式化输出。

#pragma once

#include <netinet/in.h>
#include <string>

namespace csl {

class InetAddress {
public:
    /// @brief 用端口号构造（IP 为 INADDR_ANY）
    explicit InetAddress(uint16_t port = 0, bool loopbackOnly = false);

    /// @brief 用 IP 字符串 + 端口构造
    /// @param ip   IPv4 点分十进制，如 "192.168.1.1"
    /// @param port 端口号
    InetAddress(const std::string& ip, uint16_t port);

    /// @brief 用原生 sockaddr_in 构造
    explicit InetAddress(const struct sockaddr_in& addr);

    // ---- 访问器 ----
    const struct sockaddr_in& getSockAddr() const { return addr_; }
    void setSockAddr(const struct sockaddr_in& addr) { addr_ = addr; }

    std::string toIp() const;       // "192.168.1.1"
    uint16_t toPort() const;        // 主机字节序
    std::string toIpPort() const;   // "192.168.1.1:8080"

    // ---- 静态方法 ----
    /// @brief 解析 "ip:port" 或 ":port" 或 "port"
    static InetAddress resolve(const std::string& hostport);

private:
    struct sockaddr_in addr_;
};

}  // namespace csl
