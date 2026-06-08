// web_server — HTTP WebServer
//
// 监听端口 8080，返回简单的 HTML 页面。
// 这是 cpp-server-lab 第一阶段的目标验证应用。
//
// 运行方式（Linux）：
//   ./build/csl_web_server
//   浏览器访问 http://localhost:8080

#include "csl/net/EventLoop.h"
#include "csl/net/InetAddress.h"
#include "csl/http/HttpServer.h"
#include "csl/http/HttpRequest.h"
#include "csl/http/HttpResponse.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    uint16_t port = 8080;
    if (argc > 1) {
        port = static_cast<uint16_t>(std::stoi(argv[1]));
    }

    std::cout << "=== csl HTTP WebServer ===" << std::endl;
    std::cout << "监听端口: " << port << std::endl;
    std::cout << "测试: curl http://localhost:" << port << "/" << std::endl;

    csl::EventLoop loop;
    csl::InetAddress listenAddr(port);
    csl::HttpServer server(&loop, listenAddr, "WebServer");

    server.setThreadNum(2);

    server.setHttpCallback(
        [](const csl::HttpRequest& req, csl::HttpResponse* resp) {
            std::cout << "请求: " << req.path() << std::endl;

            if (req.path() == "/" || req.path() == "/index.html") {
                resp->setStatusCode(csl::HttpResponse::k200Ok);
                resp->setStatusMessage("OK");
                resp->setContentType("text/html; charset=utf-8");
                resp->setBody(
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
                    "    <p>C++20 Reactor 网络库</p>\n"
                    "    <p>muduo 风格 · epoll LT · one loop per thread</p>\n"
                    "  </div>\n"
                    "  <p>第一阶段：MiniMuduo + WebServer ✅</p>\n"
                    "</body>\n"
                    "</html>");
            } else {
                resp->setStatusCode(csl::HttpResponse::k404NotFound);
                resp->setStatusMessage("Not Found");
                resp->setBody("<h1>404 Not Found</h1>");
            }
        });

    server.start();

    std::cout << "WebServer 已启动，按 Ctrl+C 退出" << std::endl;
    loop.loop();

    return 0;
}
