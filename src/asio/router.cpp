// Copyright 2026, cpp-server-lab

#include "csl/asio/router.h"

#include "csl/asio/http_parser.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>

namespace csl::asio {

namespace {

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool isHex(char ch) {
    return std::isxdigit(static_cast<unsigned char>(ch)) != 0;
}

int fromHex(char ch) {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }
    if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    }
    return 0;
}

std::string readFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

bool pathEscapesRoot(const std::filesystem::path& root, const std::filesystem::path& candidate) {
    std::filesystem::path relative = candidate.lexically_relative(root);
    std::string relativeText = relative.generic_string();
    return relative.empty()
        || relativeText == ".."
        || relativeText.starts_with("../")
        || relative.is_absolute();
}

}  // namespace

std::string guessMimeType(const std::filesystem::path& path) {
    std::string ext = lower(path.extension().string());
    if (ext == ".html" || ext == ".htm") {
        return "text/html; charset=utf-8";
    }
    if (ext == ".css") {
        return "text/css; charset=utf-8";
    }
    if (ext == ".js") {
        return "application/javascript; charset=utf-8";
    }
    if (ext == ".json") {
        return "application/json; charset=utf-8";
    }
    if (ext == ".txt") {
        return "text/plain; charset=utf-8";
    }
    if (ext == ".svg") {
        return "image/svg+xml";
    }
    if (ext == ".png") {
        return "image/png";
    }
    if (ext == ".jpg" || ext == ".jpeg") {
        return "image/jpeg";
    }
    if (ext == ".ico") {
        return "image/x-icon";
    }
    return "application/octet-stream";
}

std::optional<std::string> urlDecodePath(const std::string& path) {
    std::string result;
    result.reserve(path.size());
    for (std::size_t i = 0; i < path.size(); ++i) {
        char ch = path[i];
        if (ch == '%') {
            if (i + 2 >= path.size() || !isHex(path[i + 1]) || !isHex(path[i + 2])) {
                return std::nullopt;
            }
            result.push_back(static_cast<char>((fromHex(path[i + 1]) << 4) | fromHex(path[i + 2])));
            i += 2;
        } else {
            result.push_back(ch);
        }
    }
    return result;
}

Router::Router(HttpServerConfig config)
    : config_(std::move(config))
    , documentRoot_(std::filesystem::absolute(config_.documentRoot).lexically_normal()) {}

void Router::handle(const HttpRequest& request, HttpResponse* response) const {
    if (request.method != "GET" && request.method != "HEAD" && request.method != "POST") {
        response->setHeader("Allow", "GET, HEAD, POST");
        setError(response, 405, "当前仅支持 GET、HEAD、POST");
        return;
    }

    if (request.path == "/api/status") {
        handleStatus(request, response);
        return;
    }
    if (request.path == "/api/echo") {
        handleEcho(request, response);
        return;
    }

    handleStaticFile(request, response);
}

void Router::handleStatus(const HttpRequest& request, HttpResponse* response) const {
    if (request.method != "GET" && request.method != "HEAD") {
        response->setHeader("Allow", "GET, HEAD");
        setError(response, 405, "/api/status 仅支持 GET/HEAD");
        return;
    }

    response->setBody(
        "{\n"
        "  \"status\": \"ok\",\n"
        "  \"server\": \"cpp-server-lab\",\n"
        "  \"io\": \"Boost.Asio\"\n"
        "}\n",
        "application/json; charset=utf-8");
}

void Router::handleEcho(const HttpRequest& request, HttpResponse* response) const {
    if (request.method != "POST") {
        response->setHeader("Allow", "POST");
        setError(response, 405, "/api/echo 仅支持 POST");
        return;
    }

    std::string contentType = request.contentType.empty()
        ? "text/plain; charset=utf-8"
        : request.contentType;
    response->setBody(request.body, contentType);
}

void Router::handleStaticFile(const HttpRequest& request, HttpResponse* response) const {
    if (request.method != "GET" && request.method != "HEAD") {
        response->setHeader("Allow", "GET, HEAD");
        setError(response, 405, "静态资源仅支持 GET/HEAD");
        return;
    }

    auto resolved = resolveStaticPath(request.path);
    if (!resolved.has_value()) {
        setError(response, 403, "禁止访问该路径");
        return;
    }

    std::error_code ec;
    if (!std::filesystem::exists(*resolved, ec) || !std::filesystem::is_regular_file(*resolved, ec)) {
        setError(response, 404, "资源不存在");
        return;
    }

    response->setBody(readFile(*resolved), guessMimeType(*resolved));
}

std::optional<std::filesystem::path> Router::resolveStaticPath(const std::string& requestPath) const {
    auto decoded = urlDecodePath(requestPath);
    if (!decoded.has_value() || decoded->empty() || decoded->front() != '/') {
        return std::nullopt;
    }
    if (decoded->find('\0') != std::string::npos || decoded->find('\\') != std::string::npos) {
        return std::nullopt;
    }

    std::filesystem::path relative = decoded->substr(1);
    if (relative.empty()) {
        relative = config_.indexFile;
    }

    std::filesystem::path candidate = (documentRoot_ / relative).lexically_normal();
    if (std::filesystem::is_directory(candidate)) {
        candidate = (candidate / config_.indexFile).lexically_normal();
    }
    if (pathEscapesRoot(documentRoot_, candidate)) {
        return std::nullopt;
    }
    return candidate;
}

void Router::setError(HttpResponse* response, int statusCode, std::string message) const {
    response->setStatus(statusCode, reasonPhrase(statusCode));
    response->setBody(
        "<!DOCTYPE html>\n"
        "<html lang=\"zh-CN\">\n"
        "<head><meta charset=\"UTF-8\"><title>" + std::to_string(statusCode) + "</title></head>\n"
        "<body><h1>" + std::to_string(statusCode) + " " + reasonPhrase(statusCode) + "</h1><p>"
            + std::move(message) + "</p></body>\n"
        "</html>\n",
        "text/html; charset=utf-8");
}

}  // namespace csl::asio
