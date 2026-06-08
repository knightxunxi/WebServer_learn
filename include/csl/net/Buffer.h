// Copyright 2026, cpp-server-lab
// Buffer - 网络 IO 缓冲
//
// 设计意图：
//   管理 TCP 连接的输入/输出字节流，隐藏半包、粘包和部分写细节。
//   参考 muduo Buffer，采用 prependable + readable + writable 三段布局。
//
// 内存布局：
//   +-------------------+------------------+------------------+
//   | prependable bytes |  readable bytes  |  writable bytes  |
//   |                   |     (CONTENT)    |                  |
//   +-------------------+------------------+------------------+
//   |                   |                  |                  |
//   0      <=      readerIndex_   <=   writerIndex_    <=    buffer_.size()
//
// 参考：muduo/net/Buffer.h

#pragma once

#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>
#include <vector>

namespace csl {

class Buffer {
public:
    // 初始大小：1KB prependable + 1KB initial
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize  = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize)
        , readerIndex_(kCheapPrepend)
        , writerIndex_(kCheapPrepend)
    {
    }

    // ---- 容量查询 ----

    /// 可读字节数
    size_t readableBytes() const {
        return writerIndex_ - readerIndex_;
    }

    /// 可写字节数
    size_t writableBytes() const {
        return buffer_.size() - writerIndex_;
    }

    /// prependable 字节数（可用于在数据前添加 header）
    size_t prependableBytes() const {
        return readerIndex_;
    }

    // ---- 读操作 ----

    /// 获取可读区域起始指针
    const char* peek() const {
        return begin() + readerIndex_;
    }

    /// 读取 len 字节并推进读指针
    void retrieve(size_t len) {
        assert(len <= readableBytes());
        if (len < readableBytes()) {
            readerIndex_ += len;
        } else {
            retrieveAll();
        }
    }

    /// 读取到指定位置（不包含 end）
    void retrieveUntil(const char* end) {
        assert(peek() <= end);
        assert(end <= beginWrite());
        retrieve(end - peek());
    }

    /// 读取全部数据（重置读写指针）
    void retrieveAll() {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    /// 读取全部可读数据为 std::string
    std::string retrieveAllAsString() {
        return retrieveAsString(readableBytes());
    }

    /// 读取 len 字节为 std::string
    std::string retrieveAsString(size_t len) {
        assert(len <= readableBytes());
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }

    // ---- 写操作 ----

    /// 确保有 len 字节可写空间（不足时扩容）
    void ensureWritableBytes(size_t len) {
        if (writableBytes() < len) {
            makeSpace(len);
        }
        assert(writableBytes() >= len);
    }

    /// 追加数据（调用者已通过 ensureWritableBytes 确保空间充足时使用）
    void hasWritten(size_t len) {
        assert(len <= writableBytes());
        writerIndex_ += len;
    }

    /// 追加数据（从 data 拷贝 len 字节）
    void append(const char* data, size_t len) {
        ensureWritableBytes(len);
        std::copy(data, data + len, beginWrite());
        hasWritten(len);
    }

    void append(const std::string& str) {
        append(str.data(), str.size());
    }

    /// 获取可写区域起始指针
    char* beginWrite() {
        return begin() + writerIndex_;
    }

    const char* beginWrite() const {
        return begin() + writerIndex_;
    }

    // ---- prepend 操作（在可读数据前添加 header） ----

    /// 在可读区域前插入数据
    void prepend(const void* data, size_t len) {
        assert(len <= prependableBytes());
        readerIndex_ -= len;
        const char* d = static_cast<const char*>(data);
        std::copy(d, d + len, begin() + readerIndex_);
    }

    /// 从 socket fd 读取数据到缓冲
    ssize_t readFd(int fd, int* savedErrno);

    /// 扩容
    void shrink(size_t reserve) {
        Buffer other;
        other.ensureWritableBytes(readableBytes() + reserve);
        other.append(peek(), readableBytes());
        swap(other);
    }

    // ---- 交换 ----

    void swap(Buffer& rhs) {
        buffer_.swap(rhs.buffer_);
        std::swap(readerIndex_, rhs.readerIndex_);
        std::swap(writerIndex_, rhs.writerIndex_);
    }

private:
    char* begin() { return buffer_.data(); }
    const char* begin() const { return buffer_.data(); }

    void makeSpace(size_t len) {
        // 如果 prependable + writable < len + kCheapPrepend，重新分配
        if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
            buffer_.resize(writerIndex_ + len);
        } else {
            // 否则：将可读数据移到 buffer 开头，释放 prependable 空间
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_,
                      begin() + writerIndex_,
                      begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};

}  // namespace csl
