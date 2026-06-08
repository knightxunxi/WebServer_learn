// buffer_test — Buffer 单元测试

#include "csl/net/Buffer.h"

#include <cassert>
#include <cstring>
#include <iostream>

int main() {
    // ---- Test 1: 初始状态 ----
    {
        csl::Buffer buf;
        assert(buf.readableBytes() == 0);
        assert(buf.writableBytes() == csl::Buffer::kInitialSize);
        assert(buf.prependableBytes() == csl::Buffer::kCheapPrepend);
        std::cout << "[PASS] Test 1: 初始状态" << std::endl;
    }

    // ---- Test 2: append + retrieve ----
    {
        csl::Buffer buf;
        buf.append("Hello", 5);
        assert(buf.readableBytes() == 5);
        assert(strncmp(buf.peek(), "Hello", 5) == 0);

        std::string data = buf.retrieveAsString(3);
        assert(data == "Hel");
        assert(buf.readableBytes() == 2);

        data = buf.retrieveAllAsString();
        assert(data == "lo");
        assert(buf.readableBytes() == 0);

        std::cout << "[PASS] Test 2: append/retrieve" << std::endl;
    }

    // ---- Test 3: retrieveUntil ----
    {
        csl::Buffer buf;
        buf.append("AB\nCD\n", 6);
        // retrieveUntil 到第一个换行符后
        const char* newline = static_cast<const char*>(
            memchr(buf.peek(), '\n', buf.readableBytes()));
        assert(newline != nullptr);
        buf.retrieveUntil(newline + 1);

        assert(buf.readableBytes() == 3);
        assert(strncmp(buf.peek(), "CD\n", 3) == 0);

        std::cout << "[PASS] Test 3: retrieveUntil" << std::endl;
    }

    // ---- Test 4: prepend ----
    {
        csl::Buffer buf;
        buf.append("World", 5);
        buf.prepend("Hello ", 6);

        assert(buf.readableBytes() == 11);
        std::string data = buf.retrieveAllAsString();
        assert(data == "Hello World");

        std::cout << "[PASS] Test 4: prepend" << std::endl;
    }

    // ---- Test 5: 扩容 ----
    {
        csl::Buffer buf(16);  // 小初始尺寸
        std::string big(2048, 'x');
        buf.append(big.c_str(), big.size());

        assert(buf.readableBytes() == 2048);
        assert(memcmp(buf.peek(), big.c_str(), 2048) == 0);

        std::cout << "[PASS] Test 5: 扩容" << std::endl;
    }

    // ---- Test 6: retrieveAll 后空间复用 ----
    {
        csl::Buffer buf(256);
        buf.append("data", 4);
        buf.retrieveAll();
        assert(buf.readableBytes() == 0);
        assert(buf.prependableBytes() == csl::Buffer::kCheapPrepend);
        // writable 空间应可复用
        assert(buf.writableBytes() >= 256);

        std::cout << "[PASS] Test 6: retrieveAll 空间复用" << std::endl;
    }

    std::cout << "\n=== All Buffer tests passed ===" << std::endl;
    return 0;
}
