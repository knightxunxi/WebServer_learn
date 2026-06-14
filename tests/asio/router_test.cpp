// Copyright 2026, cpp-server-lab

#include "csl/asio/router.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

namespace {

csl::asio::HttpRequest makeRequest(std::string method, std::string path) {
    csl::asio::HttpRequest request;
    request.method = std::move(method);
    request.target = path;
    request.path = std::move(path);
    request.version = "HTTP/1.1";
    request.keepAlive = true;
    return request;
}

}  // namespace

int main() {
    std::filesystem::path root = std::filesystem::temp_directory_path() / "csl_router_test_public";
    std::filesystem::create_directories(root / "assets");
    {
        std::ofstream(root / "index.html") << "<h1>home</h1>";
        std::ofstream(root / "style.css") << "body{color:#111;}";
        std::ofstream(root / "assets" / "sample.svg")
            << "<svg xmlns=\"http://www.w3.org/2000/svg\"><title>sample</title></svg>";
    }

    csl::asio::HttpServerConfig config;
    config.documentRoot = root.string();
    config.indexFile = "index.html";
    csl::asio::Router router(config);

    {
        csl::asio::HttpResponse response;
        router.handle(makeRequest("GET", "/"), &response);
        assert(response.statusCode == 200);
        assert(response.contentType.find("text/html") != std::string::npos);
        assert(response.body.find("home") != std::string::npos);
    }

    {
        csl::asio::HttpResponse response;
        router.handle(makeRequest("GET", "/style.css"), &response);
        assert(response.statusCode == 200);
        assert(response.contentType.find("text/css") != std::string::npos);
    }

    {
        csl::asio::HttpResponse response;
        router.handle(makeRequest("GET", "/assets/sample.svg"), &response);
        assert(response.statusCode == 200);
        assert(response.contentType == "image/svg+xml");
        assert(response.body.find("<svg") != std::string::npos);
    }

    {
        csl::asio::HttpResponse response;
        router.handle(makeRequest("GET", "/../secret.txt"), &response);
        assert(response.statusCode == 403);
    }

    {
        csl::asio::HttpResponse response;
        router.handle(makeRequest("GET", "/missing.html"), &response);
        assert(response.statusCode == 404);
    }

    {
        csl::asio::HttpRequest request = makeRequest("POST", "/api/echo");
        request.contentType = "application/json";
        request.body = "{\"ok\":true}";
        csl::asio::HttpResponse response;
        router.handle(request, &response);
        assert(response.statusCode == 200);
        assert(response.contentType == "application/json");
        assert(response.body == "{\"ok\":true}");
    }

    std::filesystem::remove_all(root);
    return 0;
}
