// Copyright 2026, cpp-server-lab
// HttpRequest - HTTP 请求
//
// 设计意图：
//   解析 HTTP/1.1 请求行和头部，存储 method/path/version/headers/body。
//   解析逻辑在 HttpContext 中，本类仅存储数据。

#pragma once

#include <map>
#include <string>

namespace csl {

class HttpRequest {
public:
    enum Method { kInvalid, kGet, kPost, kHead, kPut, kDelete };
    enum Version { kUnknown, kHttp10, kHttp11 };

    HttpRequest() : method_(kInvalid), version_(kUnknown) {}

    // ---- 设置器 ----
    void setMethod(Method m) { method_ = m; }
    void setPath(const std::string& p) { path_ = p; }
    void setQuery(const std::string& q) { query_ = q; }
    void setVersion(Version v) { version_ = v; }
    void addHeader(const std::string& key, const std::string& value);
    void setBody(const std::string& b) { body_ = b; }

    // ---- 访问器 ----
    Method method() const { return method_; }
    const std::string& path() const { return path_; }
    const std::string& query() const { return query_; }
    Version version() const { return version_; }

    std::string methodString() const;
    std::string versionString() const;

    std::string getHeader(const std::string& key) const;
    const std::map<std::string, std::string>& headers() const { return headers_; }
    const std::string& body() const { return body_; }

private:
    Method method_;
    std::string path_;
    std::string query_;
    Version version_;
    std::map<std::string, std::string> headers_;
    std::string body_;
};

}  // namespace csl
