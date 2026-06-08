# 里程碑 #2 执行计划：基础工具模块

> 版本：v1.0 | 日期：2026-06-02  
> 参考：muduo base 模块 `D:\C1\git_open_resource\muduo-master`

---

## 目标概述

实现三个基础工具类，为后续网络模块提供通用能力：

| 类 | 目的 | 使用者 |
|---|---|---|
| `noncopyable` | 禁止拷贝语义，保护对象生命周期 | EventLoop、Poller、TcpConnection 等 |
| `Timestamp` | 统一时间表示，支持比较和格式化 | Logger、TimerQueue、压测统计 |
| `Logger` | 带时间戳和级别的诊断输出 | 全项目 |

---

## 总体约束

- C++20 标准，不使用 Boost
- 命名空间 `csl`
- 头文件放 `include/csl/base/`，实现放 `src/base/`
- 文件命名 snake_case（`noncopyable.h`、`timestamp.h`、`logger.h`）
- 注释用中文，解释设计意图和约束
- 每个步骤完成后必须能编译通过

---

## 步骤 2.1：noncopyable

### 接口设计

```cpp
// include/csl/base/noncopyable.h
namespace csl {

class noncopyable {
public:
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;

protected:
    noncopyable() = default;
    ~noncopyable() = default;
};

}  // namespace csl
```

### 设计要点

- 构造/析构为 `protected`，只能通过继承使用，不能独立实例化
- 使用 C++11 `= delete` 语法，是现代 C++ 推荐做法
- 空基类优化（EBO），不影响子类对象大小

### 验收标准

- [ ] 头文件编译通过
- [ ] 编写测试：派生类对象不可拷贝构造、不可拷贝赋值（编译期报错）
- [ ] `static_assert` 验证 `noncopyable` 子类不可拷贝

### 文件清单

```
include/csl/base/noncopyable.h   (新增)
tests/base/noncopyable_test.cpp  (新增)
```

---

## 步骤 2.2：Timestamp

### 接口设计

```cpp
// include/csl/base/timestamp.h
namespace csl {

class Timestamp {
public:
    // 构造
    Timestamp();                                    // 无效时间戳 (0)
    explicit Timestamp(int64_t microSecondsSinceEpoch);

    // 有效性
    bool valid() const;

    // 获取内部值
    int64_t microSecondsSinceEpoch() const;
    time_t secondsSinceEpoch() const;

    // 格式化输出
    std::string toString() const;                   // "1234567890.123456"
    std::string toFormattedString() const;          // "20260602 18:30:45.123456"

    // 静态工厂
    static Timestamp now();                         // 当前时间
    static Timestamp invalid();                     // 无效时间戳

    // 比较运算符
    bool operator==(const Timestamp& rhs) const;
    bool operator<(const Timestamp& rhs) const;

    // 常量
    static const int kMicroSecondsPerSecond = 1000 * 1000;

private:
    int64_t microSecondsSinceEpoch_;
};

// 自由函数
double timeDifference(Timestamp high, Timestamp low);          // 秒差 (double)
Timestamp addTime(Timestamp timestamp, double seconds);         // 加偏移

}  // namespace csl
```

### 设计要点

- 参考 muduo `Timestamp`，内部用 `int64_t` 存微秒数，`sizeof == 8`
- `now()` 使用 POSIX `gettimeofday()` 获取微秒精度当前时间
- `toString()` 格式 `"秒.微秒"`（与 muduo 一致）
- `toFormattedString()` 格式 `"YYYYMMDD HH:MM:SS.uuuuuu"`（UTC，使用 `gmtime_r`）
- 提供自由函数 `timeDifference` 和 `addTime`，方便计算超时和统计延迟
- 不依赖 Boost，手动实现 `!=`、`>`、`<=`、`>=`（通过 `operator==` 和 `operator<` 组合）

### 验收标准

- [ ] 默认构造返回无效时间戳（`valid() == false`）
- [ ] `now()` 返回有效时间戳
- [ ] `toString()` 格式正确（秒.6位微秒）
- [ ] `toFormattedString()` 格式正确（YYYYMMDD HH:MM:SS.uuuuuu）
- [ ] 比较运算符正确（等于、小于，以及组合出的不等于、大于等）
- [ ] `timeDifference()` 计算秒差正确
- [ ] `addTime()` 时间偏移正确
- [ ] 单元测试通过

### 文件清单

```
include/csl/base/timestamp.h     (新增)
src/base/timestamp.cpp           (新增)
tests/base/timestamp_test.cpp    (新增)
```

---

## 步骤 2.3：Logger

### 接口设计

```cpp
// include/csl/base/logger.h
namespace csl {

// 日志级别
enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO  = 2,
    WARN  = 3,
    ERROR = 4,
    FATAL = 5,
};

// 日志输出回调类型
using OutputFunc = void(*)(const char* msg, int len);
using FlushFunc  = void(*)();

class Logger {
public:
    // 构造时自动记录时间戳、文件名、行号
    Logger(const char* file, int line, LogLevel level, const char* func = nullptr);
    ~Logger();  // 析构时输出完整日志行

    // 返回流，用于 << 链式输出
    std::ostringstream& stream();

    // 全局设置
    static void setLogLevel(LogLevel level);
    static LogLevel logLevel();
    static void setOutput(OutputFunc func);
    static void setFlush(FlushFunc func);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace csl

// 日志宏
#define LOG_TRACE if (csl::Logger::logLevel() <= csl::LogLevel::TRACE) \
    csl::Logger(__FILE__, __LINE__, csl::LogLevel::TRACE, __func__).stream()
#define LOG_DEBUG if (csl::Logger::logLevel() <= csl::LogLevel::DEBUG) \
    csl::Logger(__FILE__, __LINE__, csl::LogLevel::DEBUG, __func__).stream()
#define LOG_INFO  if (csl::Logger::logLevel() <= csl::LogLevel::INFO)  \
    csl::Logger(__FILE__, __LINE__, csl::LogLevel::INFO).stream()
#define LOG_WARN  csl::Logger(__FILE__, __LINE__, csl::LogLevel::WARN).stream()
#define LOG_ERROR csl::Logger(__FILE__, __LINE__, csl::LogLevel::ERROR).stream()
#define LOG_FATAL csl::Logger(__FILE__, __LINE__, csl::LogLevel::FATAL).stream()
```

### 日志输出格式

```
YYYYMMDD HH:MM:SS.uuuuuu TID LEVEL MSG - filename:line\n
```

示例：
```
20260602 18:30:45.123456 12345 INFO  Socket bind success - acceptor.cpp:42
```

组成部分：

| 字段 | 来源 | 说明 |
|------|------|------|
| 时间戳 | `Timestamp::now()` | 微秒精度 UTC |
| 线程 ID | `std::this_thread::get_id()` / `syscall(SYS_gettid)` | 线程标识 |
| 日志级别 | 6 字符定宽 | `"TRACE "` `"DEBUG "` `"INFO  "` `"WARN  "` `"ERROR "` `"FATAL "` |
| 内容 | 用户 `<<` 流 | 任意 |
| 位置 | `__FILE__`（仅文件名）`__LINE__` | 源文件位置 |

### 设计要点

- 使用 Pimpl 模式（`Impl` 内部类），避免在头文件暴露 `Timestamp`、`ostringstream` 等依赖
- 默认输出到 `stderr`（避免与程序正常 stdout 混淆）
- `TRACE`/`DEBUG` 带 `__func__` 信息，`INFO` 及以上不带
- `FATAL` 级别在析构时调用 `std::abort()`（以 `_exit(1)` 替代更安全）
- `TRACE`/`DEBUG`/`INFO` 带级别过滤（if 判断），`WARN`/`ERROR`/`FATAL` 无条件输出
- 全局默认级别：编译期可选 `CSL_LOG_TRACE` / `DEBUG`，否则默认 `INFO`

### 验收标准

- [ ] 各日志级别宏编译通过
- [ ] 输出格式正确（时间戳、线程 ID、级别、内容、文件:行号）
- [ ] `LOG_TRACE`/`LOG_DEBUG`/`LOG_INFO` 受级别过滤
- [ ] `LOG_WARN`/`LOG_ERROR` 不受级别过滤
- [ ] `LOG_FATAL` 输出后程序终止
- [ ] `setLogLevel()` 可动态切换级别
- [ ] 集成测试：一个程序调用所有级别，手动验证输出格式

### 文件清单

```
include/csl/base/logger.h       (新增)
src/base/logger.cpp             (新增)
tests/base/logger_test.cpp      (新增)
```

---

## 步骤 2.4：收尾与记录

### 内容

- [ ] 更新 `tests/CMakeLists.txt`，添加三个测试的构建和 CTest 注册
- [ ] 全量构建 + 运行 CTest，确认全部通过
- [ ] 更新 `docs/05-development-log.md`，记录里程碑 #2 完成
- [ ] 清理 WIP 注释和临时代码

---

## 执行顺序

```
2.1 noncopyable (最小，先热身)
    ↓
2.2 Timestamp (独立，Logger 依赖它)
    ↓
2.3 Logger (依赖 Timestamp)
    ↓
2.4 收尾
```

---

## 预计工作量

| 步骤 | 预估耗时 | 文件数 |
|---|---|---|
| 2.1 noncopyable | 0.5h | 2 |
| 2.2 Timestamp | 1.5h | 3 |
| 2.3 Logger | 2h | 3 |
| 2.4 收尾 | 0.5h | 1 |
| **合计** | **4.5h** | **9** |

---

## 与 muduo 的差异点

| | muduo | csl (本项目) |
|---|---|---|
| **C++ 标准** | C++11 | C++20 |
| **Boost 依赖** | 有（`boost::equality_comparable` 等） | 无，手动实现 |
| **copyable 基类** | 有 | 省略（需要拷贝的类型默认就是 copyable） |
| **Timestamp 时区** | 支持时区设置 | 暂只支持 UTC，后续按需添加 |
| **Logger 架构** | LogStream + Buffer 固定栈缓冲 | std::ostringstream + Pimpl（更简洁） |
| **Logger 实现** | 白盒暴露 LogStream | Pimpl 隐藏细节 |
| **线程 ID** | `CurrentThread::tidString()` | `std::this_thread::get_id()` 或 `gettid` |
