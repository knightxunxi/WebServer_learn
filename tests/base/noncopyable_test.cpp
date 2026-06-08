// noncopyable 编译期测试
//
// 测试策略：
//   noncopyable 的核心行为在编译期体现 —— 子类对象不能被拷贝。
//   我们使用 type traits 和 SFINAE 技巧在编译期断言这一点。

#include "csl/base/noncopyable.h"

#include <type_traits>

// 辅助：检查类型是否可拷贝构造
template <typename T, typename = void>
struct is_copy_constructible_v : std::false_type {};

template <typename T>
struct is_copy_constructible_v<
    T, std::void_t<decltype(T(std::declval<const T&>()))>> : std::true_type {};

// 辅助：检查类型是否可拷贝赋值
template <typename T, typename = void>
struct is_copy_assignable_v : std::false_type {};

template <typename T>
struct is_copy_assignable_v<
    T, std::void_t<decltype(std::declval<T&>() = std::declval<const T&>())>>
    : std::true_type {};

// 测试用派生类
class TestNoCopy : public csl::noncopyable {
public:
    TestNoCopy() = default;
    explicit TestNoCopy(int v) : value(v) {}
    int value = 0;
};

int main() {
    // 核心断言：noncopyable 子类不可拷贝构造、不可拷贝赋值
    static_assert(!is_copy_constructible_v<TestNoCopy>::value,
                  "noncopyable 子类必须不可拷贝构造");
    static_assert(!is_copy_assignable_v<TestNoCopy>::value,
                  "noncopyable 子类必须不可拷贝赋值");

    // 但应该可以移动构造和移动赋值（如果不显式删除的话）
    // 这里不测试 —— noncopyable 不参与移动语义

    // 子类应该可以正常构造和使用
    {
        TestNoCopy a(42);
        // 以下两行如果取消注释，编译应报错：
        // TestNoCopy b = a;          // 拷贝构造 - 编译错误
        // TestNoCopy c; c = a;       // 拷贝赋值 - 编译错误
        (void)a;  // 避免未使用警告
    }

    return 0;
}
