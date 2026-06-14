// echo_server — Boost.Asio Echo 服务器
//
// 监听端口 9999，将收到的数据原样返回。
//
// 运行方式：
//   ./build/csl_echo_server
//   或 nc localhost 9999 连接测试

#include "csl/asio/echo_server.h"
#include "csl/platform/console.h"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    csl::platform::configureConsoleUtf8();

    uint16_t port = 9999;
    if (argc > 1) {
        port = static_cast<uint16_t>(std::stoi(argv[1]));
    }

    std::size_t threadCount = 2;
    if (argc > 2) {
        threadCount = static_cast<std::size_t>(std::stoul(argv[2]));
    }

    std::cout << "=== csl Echo Server ===" << std::endl;
    std::cout << "监听端口: " << port << std::endl;
    std::cout << "IO线程数: " << threadCount << std::endl;
    std::cout << "测试命令: nc localhost " << port << std::endl;

    std::cout << "Echo Server 已启动，按 Ctrl+C 退出" << std::endl;
    csl::asio::EchoServer server(port, threadCount);
    server.run();

    return 0;
}
