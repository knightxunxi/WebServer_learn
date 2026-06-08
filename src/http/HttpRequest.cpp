// Copyright 2026, cpp-server-lab
// HttpRequest 实现

#include "csl/http/HttpRequest.h"

#include <algorithm>
#include <cctype>

namespace csl {

void HttpRequest::addHeader(const std::string& key, const std::string& value) {
    headers_[key] = value;
}

std::string HttpRequest::getHeader(const std::string& key) const {
    auto it = headers_.find(key);
    return it != headers_.end() ? it->second : "";
}

std::string HttpRequest::methodString() const {
    switch (method_) {
        case kGet:    return "GET";
        case kPost:   return "POST";
        case kHead:   return "HEAD";
        case kPut:    return "PUT";
        case kDelete: return "DELETE";
        default:      return "UNKNOWN";
    }
}

std::string HttpRequest::versionString() const {
    switch (version_) {
        case kHttp10: return "HTTP/1.0";
        case kHttp11: return "HTTP/1.1";
        default:      return "UNKNOWN";
    }
}

}  // namespace csl
