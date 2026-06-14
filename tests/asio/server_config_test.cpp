// Copyright 2026, cpp-server-lab

#include "csl/asio/server_config.h"

#include <cassert>
#include <filesystem>
#include <fstream>

int main() {
    {
        auto config = csl::asio::loadServerConfig("not-exists.ini");
        assert(config.port == 8080);
        assert(config.threadCount == 2);
        assert(config.documentRoot == "public");
    }

    std::filesystem::path path = std::filesystem::temp_directory_path() / "csl_server_config_test.ini";
    {
        std::ofstream out(path);
        out << "[server]\n"
            << "port=18080\n"
            << "threads=4\n"
            << "document_root=www\n"
            << "index=home.html\n"
            << "keep_alive_timeout_ms=1000\n"
            << "max_header_bytes=4096\n"
            << "max_body_bytes=128\n"
            << "\n[log]\n"
            << "level=debug\n"
            << "file=logs/test.log\n"
            << "console=false\n";
    }

    auto config = csl::asio::loadServerConfig(path.string());
    assert(config.port == 18080);
    assert(config.threadCount == 4);
    assert(config.documentRoot == (path.parent_path() / "www").lexically_normal().string());
    assert(config.indexFile == "home.html");
    assert(config.keepAliveTimeoutMs == 1000);
    assert(config.maxHeaderBytes == 4096);
    assert(config.maxBodyBytes == 128);
    assert(config.log.level == "debug");
    assert(config.log.file == (path.parent_path() / "logs/test.log").lexically_normal().string());
    assert(!config.log.console);

    std::filesystem::remove(path);
    return 0;
}
