// web_server — Boost.Asio HTTP WebServer
//
// 监听端口 8080，返回简单的 HTML 页面。
//
// 运行方式：
//   ./build/csl_web_server
//   浏览器访问 http://localhost:8080

#include "csl/asio/http_server.h"
#include "csl/platform/console.h"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    csl::platform::configureConsoleUtf8();

    uint16_t port = 8080;
    if (argc > 1) {
        port = static_cast<uint16_t>(std::stoi(argv[1]));
    }

    std::size_t threadCount = 2;
    if (argc > 2) {
        threadCount = static_cast<std::size_t>(std::stoul(argv[2]));
    }

    std::cout << "=== csl HTTP WebServer ===" << std::endl;
    std::cout << "监听端口: " << port << std::endl;
    std::cout << "IO线程数: " << threadCount << std::endl;
    std::cout << "测试: curl http://localhost:" << port << "/" << std::endl;

    csl::asio::HttpServer server(port, threadCount);

    server.setRequestHandler(
        [](const csl::asio::HttpRequest& req, csl::asio::HttpResponse* resp) {
            std::cout << "请求: " << req.path << std::endl;

            if (req.path == "/" || req.path == "/index.html") {
                resp->statusCode = 200;
                resp->statusMessage = "OK";
                resp->contentType = "text/html; charset=utf-8";
                resp->body =
                    "<!DOCTYPE html>\n"
                    "<html lang=\"zh-CN\">\n"
                    "<head>\n"
                    "  <meta charset=\"UTF-8\">\n"
                    "  <title>cpp-server-lab</title>\n"
                    "  <style>\n"
                    "    body { font-family: sans-serif; padding: 2em; max-width: 800px; margin: 0 auto; }\n"
                    "    h1 { color: #333; }\n"
                    "    .info { background: #f0f0f0; padding: 1em; border-radius: 4px; }\n"
                    "  </style>\n"
                    "</head>\n"
                    "<body>\n"
                    "  <h1>cpp-server-lab WebServer</h1>\n"
                    "  <div class=\"info\">\n"
                    "    <p>C++20 Boost.Asio 异步网络服务</p>\n"
                    "    <p>跨平台 IO · 回调式异步 · 多线程 io_context</p>\n"
                    "  </div>\n"
                    "  <p>当前主线：Boost.Asio 跨平台网络实现</p>\n"
                    "</body>\n"
                    "</html>";
            } else {
                resp->statusCode = 404;
                resp->statusMessage = "Not Found";
                resp->body = "<h1>404 Not Found</h1>";
            }
        });

    std::cout << "WebServer 已启动，按 Ctrl+C 退出" << std::endl;
    server.run();

    return 0;
}
