// web_server — Boost.Asio HTTP WebServer
//
// 运行方式：
//   ./build/csl_web_server
//   ./build/csl_web_server config/server.ini

#include "csl/asio/http_server.h"
#include "csl/asio/logger.h"
#include "csl/asio/router.h"
#include "csl/asio/server_config.h"
#include "csl/platform/console.h"

#include <exception>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    csl::platform::configureConsoleUtf8();

    const std::string configPath = argc > 1 ? argv[1] : "config/server.ini";

    try {
        csl::asio::HttpServerConfig config = csl::asio::loadServerConfig(configPath);
        csl::asio::Router router(config);
        csl::asio::HttpServer server(config);

        server.setRequestHandler(
            [&router](const csl::asio::HttpRequest& req, csl::asio::HttpResponse* resp) {
                router.handle(req, resp);
            });

        std::cout << "=== csl HTTP WebServer ===" << std::endl;
        std::cout << "配置文件: " << configPath << std::endl;
        std::cout << "监听端口: " << config.port << std::endl;
        std::cout << "IO线程数: " << config.threadCount << std::endl;
        std::cout << "静态目录: " << config.documentRoot << std::endl;
        std::cout << "测试: http://localhost:" << config.port << "/" << std::endl;
        std::cout << "WebServer 已启动，按 Ctrl+C 退出" << std::endl;

        server.run();
    } catch (const std::exception& ex) {
        std::cerr << "WebServer 启动失败: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
