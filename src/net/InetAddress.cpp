// Copyright 2026, cpp-server-lab
// InetAddress 实现

#include "csl/net/InetAddress.h"

#include <cstring>
#include <arpa/inet.h>

namespace csl {

// ===== 构造 =====

InetAddress::InetAddress(uint16_t port, bool loopbackOnly) {
    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = htonl(loopbackOnly ? INADDR_LOOPBACK : INADDR_ANY);
    addr_.sin_port = htons(port);
}

InetAddress::InetAddress(const std::string& ip, uint16_t port) {
    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    if (::inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr) <= 0) {
        // 解析失败，回退到 INADDR_ANY
        addr_.sin_addr.s_addr = htonl(INADDR_ANY);
    }
}

InetAddress::InetAddress(const struct sockaddr_in& addr)
    : addr_(addr) {}

// ===== 格式化 =====

std::string InetAddress::toIp() const {
    char buf[INET_ADDRSTRLEN] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    return buf;
}

uint16_t InetAddress::toPort() const {
    return ntohs(addr_.sin_port);
}

std::string InetAddress::toIpPort() const {
    char buf[32] = {0};
    snprintf(buf, sizeof(buf), "%s:%u", toIp().c_str(), toPort());
    return buf;
}

// ===== 静态解析 =====

InetAddress InetAddress::resolve(const std::string& hostport) {
    auto colon = hostport.find(':');
    if (colon == std::string::npos) {
        // 仅端口号
        uint16_t port = static_cast<uint16_t>(std::stoi(hostport));
        return InetAddress(port);
    } else {
        std::string ip = hostport.substr(0, colon);
        uint16_t port = static_cast<uint16_t>(
            std::stoi(hostport.substr(colon + 1)));
        return InetAddress(ip, port);
    }
}

}  // namespace csl
