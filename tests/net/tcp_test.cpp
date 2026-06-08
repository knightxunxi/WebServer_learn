// tcp_test — InetAddress / Socket / Acceptor / TcpConnection 集成测试
//
// 运行方式（Linux）：
//   cmake --build build && ./build/tests/csl_tcp_test

#include "csl/net/InetAddress.h"
#include "csl/net/Socket.h"
#include "csl/net/Acceptor.h"
#include "csl/net/TcpConnection.h"
#include "csl/net/EventLoop.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <thread>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// ===== Test 1: InetAddress 基本功能 =====
static void testInetAddress() {
    // 端口构造
    csl::InetAddress addr1(8080);
    assert(addr1.toPort() == 8080);
    assert(addr1.toIpPort() == "0.0.0.0:8080");

    // IP + 端口构造
    csl::InetAddress addr2("127.0.0.1", 9527);
    assert(addr2.toIp() == "127.0.0.1");
    assert(addr2.toPort() == 9527);

    // resolve
    csl::InetAddress addr3 = csl::InetAddress::resolve("0.0.0.0:4396");
    assert(addr3.toPort() == 4396);

    csl::InetAddress addr4 = csl::InetAddress::resolve("9999");
    assert(addr4.toPort() == 9999);

    std::cout << "[PASS] Test 1: InetAddress" << std::endl;
}

// ===== Test 2: Socket 基本操作 =====
static void testSocket() {
    int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    assert(fd >= 0);

    csl::Socket sock(fd);
    sock.setReuseAddr(true);
    sock.setTcpNoDelay(true);
    sock.setKeepAlive(true);

    csl::InetAddress addr(12345);
    sock.bindAddress(addr);
    sock.listen();

    std::cout << "[PASS] Test 2: Socket bind/listen" << std::endl;
}

// ===== Test 3: Acceptor + TcpConnection 集成 =====
static void testAcceptorAndTcpConnection() {
    csl::EventLoop loop;
    csl::InetAddress listenAddr(12346);
    csl::Acceptor acceptor(&loop, listenAddr, false);

    bool newConnReceived = false;
    bool messageReceived = false;
    std::string receivedData;

    acceptor.setNewConnectionCallback(
        [&](int sockfd, const csl::InetAddress& peerAddr) {
            newConnReceived = true;
            auto conn = std::make_shared<csl::TcpConnection>(
                &loop, "test-conn", sockfd,
                listenAddr, peerAddr);

            conn->setMessageCallback(
                [&](const std::shared_ptr<csl::TcpConnection>& c,
                    csl::Timestamp) {
                    messageReceived = true;
                    receivedData = c->inputBuffer();
                    loop.quit();
                });

            conn->setConnectionCallback(
                [&](const std::shared_ptr<csl::TcpConnection>& c) {
                    if (c->connected()) {
                        // 连接建立
                    }
                });

            conn->connectEstablished();
        });

    acceptor.listen();

    // 启动子线程作为客户端连接
    std::thread client([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

        int clientfd = ::socket(AF_INET, SOCK_STREAM, 0);
        assert(clientfd >= 0);

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(12346);
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

        int ret = ::connect(clientfd, (struct sockaddr*)&addr, sizeof(addr));
        if (ret < 0) {
            perror("connect failed");
            std::abort();
        }

        // 发送数据
        const char* msg = "Hello, Server!";
        ssize_t n = ::write(clientfd, msg, strlen(msg));
        assert(n > 0);

        // 等待服务端处理
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        ::close(clientfd);
    });

    loop.loop();
    client.join();

    assert(newConnReceived);
    assert(messageReceived);
    assert(receivedData == "Hello, Server!");

    std::cout << "[PASS] Test 3: Acceptor + TcpConnection" << std::endl;
}

// ===== Test 4: TcpConnection send/echo =====
static void testTcpSend() {
    csl::EventLoop loop;
    csl::InetAddress listenAddr(12347);
    csl::Acceptor acceptor(&loop, listenAddr, false);

    std::shared_ptr<csl::TcpConnection> serverConn;

    acceptor.setNewConnectionCallback(
        [&](int sockfd, const csl::InetAddress& peerAddr) {
            serverConn = std::make_shared<csl::TcpConnection>(
                &loop, "echo-conn", sockfd,
                listenAddr, peerAddr);

            serverConn->setMessageCallback(
                [&](const std::shared_ptr<csl::TcpConnection>& conn,
                    csl::Timestamp) {
                    std::string data = conn->inputBuffer();
                    conn->send(data);  // echo 回去
                });

            serverConn->connectEstablished();
        });

    acceptor.listen();

    std::thread client([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

        int clientfd = ::socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(12347);
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
        ::connect(clientfd, (struct sockaddr*)&addr, sizeof(addr));

        const char* msg = "echo-me";
        ::write(clientfd, msg, strlen(msg));

        // 读取 echo 响应
        char buf[256] = {0};
        ssize_t n = ::read(clientfd, buf, sizeof(buf) - 1);
        assert(n > 0);
        buf[n] = '\0';

        assert(strcmp(buf, "echo-me") == 0);

        ::close(clientfd);
        loop.quit();
    });

    loop.loop();
    client.join();

    std::cout << "[PASS] Test 4: TcpConnection echo" << std::endl;
}

int main() {
    testInetAddress();
    testSocket();
    testAcceptorAndTcpConnection();
    testTcpSend();

    std::cout << "\n=== All TCP tests passed ===" << std::endl;
    return 0;
}
