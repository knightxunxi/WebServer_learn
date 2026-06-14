// Copyright 2026, cpp-server-lab

#include "csl/asio/http_server.h"

#include <cassert>
#include <string>

int main() {
    csl::asio::HttpResponse response;
    response.body = "hello";
    response.close = true;

    const std::string raw = csl::asio::serializeResponse(response);

    assert(raw.find("HTTP/1.1 200 OK\r\n") == 0);
    assert(raw.find("Connection: close\r\n") != std::string::npos);
    assert(raw.find("Content-Type: text/html; charset=utf-8\r\n") != std::string::npos);
    assert(raw.find("Content-Length: 5\r\n") != std::string::npos);
    assert(raw.ends_with("\r\n\r\nhello"));

    return 0;
}
