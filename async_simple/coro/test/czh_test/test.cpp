//
// Created by viruser on 2024/8/26.
//

#include <spdlog/spdlog.h>
#include <string>
#include <type_traits>
#include <utility>

template <class T>
concept HasCoAwaitMethod =
    requires(T&& awaitable) { awaitable.coAwait(nullptr); };

template <class T>
struct CheckHasCoAwaitMethod {
    template <class U, typename = decltype(std::declval<U>().coAwait(nullptr))>
    static constexpr std::true_type check(std::nullptr_t) {
        return std::true_type{};
    }
    template <class U>
    static constexpr std::false_type check(...) {
        return std::false_type{};
    }

    static constexpr bool value = check<T>(nullptr);
    static constexpr bool value2 = decltype(check<T>(nullptr))::value;
};

class Executor;
class A {
public:
    bool coAwait(Executor* ex) { return true; }
};
class B {
public:
    int value = 0;
};

struct SimpleAwaiter {
    std::string name;
    SimpleAwaiter(std::string n) : name(n) {}
};

class C {
public:
    auto operator co_await() { return SimpleAwaiter("C Member"); }
};

auto operator co_await(class A) { return SimpleAwaiter("A Global"); }

void test_type_traits() {
    if constexpr (CheckHasCoAwaitMethod<B>::value2) {
        spdlog::info("HasCoAwaitMethod true");
    } else {
        spdlog::info("HasCoAwaitMethod false");
    }
}

#include <sstream>
#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/SyncAwait.h"

std::string get_current_thread_id() {
    std::ostringstream oss;
    oss << std::this_thread::get_id()  ;  // 获取当前线程ID
    return oss.str();
}

using namespace async_simple::coro;

Lazy<int> foo() {
    spdlog::info("call foo id:{}", get_current_thread_id());
    co_return 43;
}

// 一个lazy 就是包含promise对象和对应的awaiter对象
void foo_use2() {
    spdlog::info("call foo_use2 id:{}", get_current_thread_id());
    // foo() 开启协程后-->调用syncAwait 开启detached协程 通过co_await foo的promise对应的awaiter对象来执行操作,其中主要通过lazy awaiter类型中
    auto val = async_simple::coro::syncAwait(foo());
    spdlog::info("foo_use2 value:{}", val);
}

int main() {
    //    test_type_traits();
    foo_use2();
    return 0;
}