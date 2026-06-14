// Copyright 2026, cpp-server-lab

#include "csl/asio/http_parser.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>

namespace csl::asio {

namespace {

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string trim(std::string value) {
    auto notSpace = [](unsigned char ch) {
        return !std::isspace(ch);
    };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
    return value;
}

bool isTokenChar(unsigned char ch) {
    return std::isalnum(ch) || ch == '-' || ch == '_' || ch == '.';
}

bool isAllowedContentType(const std::string& value) {
    std::string normalized = lower(value);
    auto semicolon = normalized.find(';');
    if (semicolon != std::string::npos) {
        normalized = trim(normalized.substr(0, semicolon));
    }
    return normalized == "text/plain"
        || normalized == "application/json"
        || normalized == "application/x-www-form-urlencoded";
}

bool parseContentLength(const std::string& value, std::size_t* result) {
    try {
        std::size_t consumed = 0;
        std::size_t parsed = std::stoull(trim(value), &consumed);
        if (consumed != trim(value).size()) {
            return false;
        }
        *result = parsed;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

}  // namespace

std::string HttpRequest::header(const std::string& name) const {
    auto it = headers.find(lower(name));
    if (it == headers.end()) {
        return {};
    }
    return it->second;
}

bool HttpRequest::hasHeader(const std::string& name) const {
    return headers.find(lower(name)) != headers.end();
}

HttpParseResult parseHttpRequestHead(const std::string& rawHeaders,
                                     const HttpServerConfig& config) {
    HttpParseResult result;
    if (rawHeaders.size() > config.maxHeaderBytes) {
        result.status = HttpParseStatus::HeaderTooLarge;
        result.message = "request header too large";
        return result;
    }

    std::istringstream stream(rawHeaders);
    std::string requestLine;
    if (!std::getline(stream, requestLine)) {
        result.status = HttpParseStatus::BadRequest;
        result.message = "missing request line";
        return result;
    }
    if (!requestLine.empty() && requestLine.back() == '\r') {
        requestLine.pop_back();
    }

    std::istringstream requestLineStream(requestLine);
    requestLineStream >> result.request.method >> result.request.target >> result.request.version;
    std::string extra;
    if (result.request.method.empty()
        || result.request.target.empty()
        || result.request.version.empty()
        || (requestLineStream >> extra)) {
        result.status = HttpParseStatus::BadRequest;
        result.message = "bad request line";
        return result;
    }

    if (!std::all_of(result.request.method.begin(), result.request.method.end(), [](unsigned char ch) {
            return isTokenChar(ch);
        })) {
        result.status = HttpParseStatus::BadRequest;
        result.message = "bad method token";
        return result;
    }

    auto queryPos = result.request.target.find('?');
    result.request.path = result.request.target.substr(0, queryPos);
    if (queryPos != std::string::npos) {
        result.request.query = result.request.target.substr(queryPos + 1);
    }
    if (result.request.path.empty() || result.request.path.front() != '/') {
        result.status = HttpParseStatus::BadRequest;
        result.message = "bad request target";
        return result;
    }

    bool connectionClose = false;
    bool connectionKeepAlive = false;
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            break;
        }

        auto colon = line.find(':');
        if (colon == std::string::npos) {
            result.status = HttpParseStatus::BadRequest;
            result.message = "bad header line";
            return result;
        }

        std::string key = lower(trim(line.substr(0, colon)));
        std::string value = trim(line.substr(colon + 1));
        if (key.empty()) {
            result.status = HttpParseStatus::BadRequest;
            result.message = "empty header name";
            return result;
        }
        result.request.headers[key] = value;

        if (key == "connection") {
            std::string normalizedValue = lower(value);
            connectionClose = normalizedValue.find("close") != std::string::npos;
            connectionKeepAlive = normalizedValue.find("keep-alive") != std::string::npos;
        } else if (key == "content-length") {
            if (!parseContentLength(value, &result.request.contentLength)) {
                result.status = HttpParseStatus::BadRequest;
                result.message = "bad content-length";
                return result;
            }
            if (result.request.contentLength > config.maxBodyBytes) {
                result.status = HttpParseStatus::BodyTooLarge;
                result.message = "request body too large";
                return result;
            }
        } else if (key == "content-type") {
            result.request.contentType = value;
        }
    }

    if (result.request.version == "HTTP/1.1") {
        result.request.keepAlive = !connectionClose;
    } else if (result.request.version == "HTTP/1.0") {
        result.request.keepAlive = connectionKeepAlive;
    } else {
        result.status = HttpParseStatus::BadRequest;
        result.message = "unsupported http version";
        return result;
    }

    if (result.request.method == "POST"
        && result.request.contentLength > 0
        && !isAllowedContentType(result.request.contentType)) {
        result.status = HttpParseStatus::UnsupportedMediaType;
        result.message = "unsupported content-type";
        return result;
    }

    result.status = HttpParseStatus::Ok;
    return result;
}

HttpParseStatus assignRequestBody(HttpRequest* request,
                                  std::string body,
                                  const HttpServerConfig& config) {
    if (body.size() > config.maxBodyBytes) {
        return HttpParseStatus::BodyTooLarge;
    }
    if (request->contentLength != body.size()) {
        return HttpParseStatus::BadRequest;
    }
    if (request->method == "POST"
        && !body.empty()
        && !isAllowedContentType(request->contentType)) {
        return HttpParseStatus::UnsupportedMediaType;
    }
    request->body = std::move(body);
    return HttpParseStatus::Ok;
}

int statusCodeForParseError(HttpParseStatus status) {
    switch (status) {
        case HttpParseStatus::Ok: return 200;
        case HttpParseStatus::BadRequest: return 400;
        case HttpParseStatus::HeaderTooLarge: return 431;
        case HttpParseStatus::BodyTooLarge: return 413;
        case HttpParseStatus::UnsupportedMediaType: return 415;
    }
    return 400;
}

std::string reasonPhrase(int statusCode) {
    switch (statusCode) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Payload Too Large";
        case 415: return "Unsupported Media Type";
        case 431: return "Request Header Fields Too Large";
        case 500: return "Internal Server Error";
        default: return "Unknown";
    }
}

}  // namespace csl::asio
