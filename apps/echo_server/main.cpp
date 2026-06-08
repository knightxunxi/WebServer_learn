// echo_server — 单线程 Echo 服务器
//
// 监听端口 9999，将收到的数据原样返回。
// 这是 cpp-server-lab 第一个可运行的网络应用程序。
//
// 运行方式（Linux）：
//   ./build/csl_echo_server
//   或 nc localhost 9999 连接测试

#include "csl/net/EventLoop.h"
#include "csl/net/TcpServer.h"
#include "csl/net/TcpConnection.h"
#include "csl/net/InetAddress.h"
#include "csl/net/Buffer.h"
#include "csl/base/timestamp.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    uint16_t port = 9999;
    if (argc > 1) {
        port = static_cast<uint16_t>(std::stoi(argv[1]));
    }

    std::cout << "=== csl Echo Server ===" << std::endl;
    std::cout << "监听端口: " << port << std::endl;
    std::cout << "测试命令: nc localhost " << port << std::endl;

    csl::EventLoop loop;
    csl::InetAddress listenAddr(port);
    csl::TcpServer server(&loop, listenAddr, "EchoServer");

    // 设置 IO 线程数（0 = 单线程，>=1 = 多线程）
    server.setThreadNum(2);  // 2 个 IO 线程

    // 消息回调：原样返回
    server.setMessageCallback(
        [](const csl::TcpServer::TcpConnectionPtr& conn,
           csl::Buffer* buf,
           csl::Timestamp receiveTime) {
            (void)receiveTime;
            std::string msg = buf->retrieveAllAsString();
            conn->send(msg);
        });

    // 连接回调：打印连接/断开信息
    server.setConnectionCallback(
        [](const csl::TcpServer::TcpConnectionPtr& conn) {
            if (conn->connected()) {
                std::cout << "[" << conn->name() << "] 新连接来自 "
                          << conn->peerAddress().toIpPort() << std::endl;
            } else {
                std::cout << "[" << conn->name() << "] 连接断开" << std::endl;
            }
        });

    server.start();

    std::cout << "Echo Server 已启动，按 Ctrl+C 退出" << std::endl;
    loop.loop();

    return 0;
}
