// Copyright 2026, cpp-server-lab

#include "csl/asio/http_parser.h"

#include <cassert>
#include <string>

int main() {
    csl::asio::HttpServerConfig config;
    std::string body = "{\"msg\":\"hi\"}";
    std::string raw =
        "POST /api/echo?debug=1 HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Connection: keep-alive\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "\r\n";

    auto parsed = csl::asio::parseHttpRequestHead(raw, config);
    assert(parsed.status == csl::asio::HttpParseStatus::Ok);
    assert(parsed.request.method == "POST");
    assert(parsed.request.path == "/api/echo");
    assert(parsed.request.query == "debug=1");
    assert(parsed.request.header("HOST") == "localhost");
    assert(parsed.request.keepAlive);
    assert(parsed.request.contentLength == body.size());

    auto bodyStatus = csl::asio::assignRequestBody(&parsed.request, body, config);
    assert(bodyStatus == csl::asio::HttpParseStatus::Ok);
    assert(parsed.request.body == "{\"msg\":\"hi\"}");

    config.maxBodyBytes = 3;
    auto tooLarge = csl::asio::parseHttpRequestHead(raw, config);
    assert(tooLarge.status == csl::asio::HttpParseStatus::BodyTooLarge);

    config.maxBodyBytes = 1024;
    std::string unsupported =
        "POST /api/echo HTTP/1.1\r\n"
        "Content-Type: multipart/form-data\r\n"
        "Content-Length: 4\r\n"
        "\r\n";
    auto badType = csl::asio::parseHttpRequestHead(unsupported, config);
    assert(badType.status == csl::asio::HttpParseStatus::UnsupportedMediaType);

    std::string badRequest = "GET /missing-version\r\n\r\n";
    auto bad = csl::asio::parseHttpRequestHead(badRequest, config);
    assert(bad.status == csl::asio::HttpParseStatus::BadRequest);

    return 0;
}
