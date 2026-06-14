// Copyright 2026, cpp-server-lab

#include "csl/asio/server_config.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace csl::asio {

namespace {

using IniData = std::unordered_map<std::string, std::unordered_map<std::string, std::string>>;

std::string trim(std::string value) {
    auto notSpace = [](unsigned char ch) {
        return !std::isspace(ch);
    };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
    return value;
}

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::size_t parseSize(const IniData& ini,
                      const std::string& section,
                      const std::string& key,
                      std::size_t defaultValue) {
    auto sectionIt = ini.find(section);
    if (sectionIt == ini.end()) {
        return defaultValue;
    }
    auto valueIt = sectionIt->second.find(key);
    if (valueIt == sectionIt->second.end()) {
        return defaultValue;
    }

    try {
        std::size_t consumed = 0;
        std::size_t result = std::stoull(valueIt->second, &consumed);
        if (consumed != valueIt->second.size()) {
            throw std::invalid_argument("trailing characters");
        }
        return result;
    } catch (const std::exception&) {
        throw std::runtime_error("配置项 " + section + "." + key + " 必须是非负整数");
    }
}

std::string parseString(const IniData& ini,
                        const std::string& section,
                        const std::string& key,
                        const std::string& defaultValue) {
    auto sectionIt = ini.find(section);
    if (sectionIt == ini.end()) {
        return defaultValue;
    }
    auto valueIt = sectionIt->second.find(key);
    if (valueIt == sectionIt->second.end()) {
        return defaultValue;
    }
    return valueIt->second;
}

bool parseBool(const IniData& ini,
               const std::string& section,
               const std::string& key,
               bool defaultValue) {
    std::string value = lower(parseString(ini, section, key, defaultValue ? "true" : "false"));
    if (value == "true" || value == "1" || value == "yes" || value == "on") {
        return true;
    }
    if (value == "false" || value == "0" || value == "no" || value == "off") {
        return false;
    }
    throw std::runtime_error("配置项 " + section + "." + key + " 必须是布尔值");
}

IniData parseIni(std::istream& input) {
    IniData result;
    std::string section;
    std::string line;
    std::size_t lineNo = 0;

    while (std::getline(input, line)) {
        ++lineNo;
        line = trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }

        if (line.front() == '[' && line.back() == ']') {
            section = lower(trim(line.substr(1, line.size() - 2)));
            if (section.empty()) {
                throw std::runtime_error("INI 第 " + std::to_string(lineNo) + " 行 section 为空");
            }
            continue;
        }

        auto equals = line.find('=');
        if (equals == std::string::npos) {
            throw std::runtime_error("INI 第 " + std::to_string(lineNo) + " 行缺少 '='");
        }
        if (section.empty()) {
            throw std::runtime_error("INI 第 " + std::to_string(lineNo) + " 行缺少 section");
        }

        std::string key = lower(trim(line.substr(0, equals)));
        std::string value = trim(line.substr(equals + 1));
        if (key.empty()) {
            throw std::runtime_error("INI 第 " + std::to_string(lineNo) + " 行 key 为空");
        }
        result[section][key] = value;
    }

    return result;
}

std::optional<std::filesystem::path> findConfigPath(const std::string& path) {
    std::filesystem::path requested(path);
    if (std::filesystem::exists(requested)) {
        return std::filesystem::absolute(requested).lexically_normal();
    }

    if (requested.is_absolute()) {
        return std::nullopt;
    }

    std::filesystem::path base = std::filesystem::current_path();
    for (int i = 0; i < 3; ++i) {
        std::filesystem::path candidate = (base / requested).lexically_normal();
        if (std::filesystem::exists(candidate)) {
            return std::filesystem::absolute(candidate).lexically_normal();
        }
        base /= "..";
    }
    return std::nullopt;
}

std::filesystem::path configBaseDirectory(const std::filesystem::path& configPath) {
    std::filesystem::path configDir = configPath.parent_path();
    if (lower(configDir.filename().string()) == "config" && configDir.has_parent_path()) {
        return configDir.parent_path();
    }
    return configDir;
}

std::string resolveRelativePath(const std::filesystem::path& base, const std::string& value) {
    std::filesystem::path path(value);
    if (path.is_absolute()) {
        return path.lexically_normal().string();
    }
    return (base / path).lexically_normal().string();
}

}  // namespace

HttpServerConfig loadServerConfig(const std::string& path) {
    HttpServerConfig config;

    auto resolvedConfigPath = findConfigPath(path);
    if (!resolvedConfigPath.has_value()) {
        return config;
    }

    std::ifstream input(*resolvedConfigPath);
    if (!input.is_open()) {
        return config;
    }

    IniData ini = parseIni(input);

    std::size_t port = parseSize(ini, "server", "port", config.port);
    if (port == 0 || port > 65535) {
        throw std::runtime_error("配置项 server.port 必须在 1~65535 范围内");
    }
    config.port = static_cast<unsigned short>(port);

    config.threadCount = std::max<std::size_t>(1, parseSize(ini, "server", "threads", config.threadCount));
    config.documentRoot = parseString(ini, "server", "document_root", config.documentRoot);
    config.indexFile = parseString(ini, "server", "index", config.indexFile);
    config.keepAliveTimeoutMs = parseSize(ini, "server", "keep_alive_timeout_ms", config.keepAliveTimeoutMs);
    config.maxHeaderBytes = parseSize(ini, "server", "max_header_bytes", config.maxHeaderBytes);
    config.maxBodyBytes = parseSize(ini, "server", "max_body_bytes", config.maxBodyBytes);

    config.log.level = parseString(ini, "log", "level", config.log.level);
    config.log.file = parseString(ini, "log", "file", config.log.file);
    config.log.console = parseBool(ini, "log", "console", config.log.console);

    std::filesystem::path base = configBaseDirectory(*resolvedConfigPath);
    config.documentRoot = resolveRelativePath(base, config.documentRoot);
    if (!config.log.file.empty()) {
        config.log.file = resolveRelativePath(base, config.log.file);
    }

    if (config.documentRoot.empty()) {
        throw std::runtime_error("配置项 server.document_root 不能为空");
    }
    if (config.indexFile.empty()) {
        throw std::runtime_error("配置项 server.index 不能为空");
    }
    if (config.maxHeaderBytes == 0) {
        throw std::runtime_error("配置项 server.max_header_bytes 必须大于 0");
    }

    return config;
}

void prepareRuntimeDirectories(const HttpServerConfig& config) {
    std::filesystem::create_directories(config.documentRoot);

    if (!config.log.file.empty()) {
        std::filesystem::path logPath(config.log.file);
        if (logPath.has_parent_path()) {
            std::filesystem::create_directories(logPath.parent_path());
        }
    }
}

}  // namespace csl::asio
