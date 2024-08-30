//
// Created by viruser on 2024/8/16.
//

#include <spdlog/spdlog.h>
#include <coroutine>
#include <iostream>
#include <memory>

template <bool READY>
struct Awaiter {
    bool await_ready() noexcept {
        std::cout << "await_ready: " << READY << "addr" << std::addressof(*this)
                  << std::endl;

        return READY;
    }
    void await_resume() noexcept { std::cout << "await_resume" << std::endl; }
    bool await_suspend(std::coroutine_handle<> h) noexcept {
        std::cout << "await_suspend " << std::endl;
        return true;
    }
};

// Promise规范
struct TaskPromise {
    struct promise_type {
        TaskPromise get_return_object() {
            std::cout << "get_return_object"
                      << "this:" << std::addressof(*this) << std::endl;
            return TaskPromise{
                std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        Awaiter<false> initial_suspend() noexcept {
            std::cout << "initial_suspend" << std::endl;
            return {};
        }
        Awaiter<true> final_suspend() noexcept {
            std::cout << "final_suspend" << std::endl;
            return {};
        }
        void unhandled_exception() {
            std::cout << "unhandled_exception" << std::endl;
        }
        void return_void() noexcept { std::cout << "return_void" << std::endl; }
    };
    void resume() {
        std::cout << "resume" << std::endl;
        handle.resume();
    }
    std::coroutine_handle<promise_type> handle;
};

// 协程函数定义：就是如果一个函数的返回值是一个符合Promise规范的类（promise_type），并且在这个函数中使用了co_return，co_yield，co_await中的一个或多个，那么这个函数就是一个协程函数。
TaskPromise task_func() {
    std::cout << "task first run" << std::endl;
    co_await std::suspend_never{};
    std::cout << "task resume" << std::endl;
}

#include <coroutine>
#include <iostream>
#include <memory>
#include <thread>
class IntReader {
public:
    IntReader(int value) : value_(value) {}
    bool await_ready() { return false; }

    auto await_suspend(std::coroutine_handle<> handle) {
        spdlog::info("await_suspend");
        std::thread thread([this, handle]() {
            static int rand_num = 0;
            value_ = rand_num++;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            spdlog::info("await_suspend call resume");
            handle.resume();
        });

        thread.detach();
        return handle;
    }

    int await_resume() {
        spdlog::info("call await_resume");
        return value_;
    }

private:
    int value_;
};

class Task {
public:
    class promise_type {
    public:
        promise_type() : value_(std::make_shared<int>(0)) {}
        Task get_return_object() {
            std::cout << "get_return_object" << std::endl;
            return Task(
                std::coroutine_handle<promise_type>::from_promise(*this));
        }

        void return_value(int value) {
            spdlog::info("call co_return");
            *value_ = value;
        }

        int get() { return *value_; }

        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void unhandled_exception() {}
        ~promise_type() { spdlog::info("call promise_type"); }

    private:
        std::shared_ptr<int> value_;
    };

public:
    Task(std::coroutine_handle<promise_type> handle) : coro_handle(handle){};

    bool is_done() { return coro_handle.done(); }

    int get() { return coro_handle.promise().get(); }

private:
    std::coroutine_handle<promise_type> coro_handle;
    bool m_alive = true;
};

Task GetInt() {
    IntReader reader1{1};
    int total = -1;
    total = co_await reader1;
    spdlog::info("reader1 value:{}", total);

    IntReader reader2 = 2;
    total += co_await reader2;
    spdlog::info("reader2 value:{}", total);

    IntReader reader3(3);
    total += co_await reader3;
    spdlog::info("reader3 value:{}", total);
    co_return total;
}

void test_coro1() {
    std::cout << "before task_func" << std::endl;
    task_func();
    std::cout << "task_func" << std::endl;
    //    promise.resume();
    std::cout << "after task_func" << std::endl;
}

void test_coro2() {
    auto task = GetInt();
    while (!task.is_done()) {
        spdlog::info("log");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    if (task.is_done()) {
        spdlog::info("get :{}", task.get());
    }
}
//
// void test_move_func(){
//    auto ptr = std::make_unique<int>(1);
//    auto func = [ptr=std::move(ptr)](){};
//    std::function<void()> f = func;
//}




#include <coroutine>
#include <iostream>
#include <thread>

struct Tmp {
    Tmp() { spdlog::info("Tmp"); }
    ~Tmp() { spdlog::info("~Tmp"); }
};

struct TopTask {
    std::coroutine_handle<> handle_;

    struct promise_type {
        TopTask get_return_object() noexcept {
            auto handle =
                std::coroutine_handle<promise_type>::from_promise(*this);
            std::cout << "TopTask handle address: " << handle.address()
                      << std::endl;
            return TopTask{.handle_ = handle};
        }

        std::suspend_always initial_suspend() const noexcept {
            spdlog::info("TopTask initial_suspend");
            return {};
        }

        std::suspend_never final_suspend() const noexcept {
            spdlog::info("TopTask final_suspend");
            return {};
        }

        void unhandled_exception() noexcept {}

        void return_void() noexcept {}
    };
};

struct MiddleTask {
    struct FinalAwaiter;
    struct promise_type;

    std::coroutine_handle<promise_type> handle_;

    ~MiddleTask() {
        if (handle_) {
            handle_.destroy();
        }
        std::cout << "MiddleTask  destructed" << std::endl;
    }

    struct promise_type {
        std::coroutine_handle<> continuation_;

        MiddleTask get_return_object() noexcept {
            auto handle =
                std::coroutine_handle<promise_type>::from_promise(*this);
            std::cout << "MiddleTask handle address: " << handle.address()
                      << std::endl;
            return MiddleTask{.handle_ = handle};
        }

        std::suspend_never initial_suspend() const noexcept {
            spdlog::info("MiddleTask initial_suspend");
            return {};
        }

        FinalAwaiter final_suspend() noexcept {
            spdlog::info("MiddleTask final_suspend");
            auto h = std::coroutine_handle<promise_type>::from_promise(*this);
            return {.handle_ = h};
        }

        void unhandled_exception() noexcept {}

        void return_void() noexcept {}
    };

    struct FinalAwaiter {
        std::coroutine_handle<promise_type> handle_;

        bool await_ready() const noexcept { return false; }

        void await_suspend(std::coroutine_handle<> cont) noexcept {
            std::cout << "MiddleTask FinalAwaiter await_suspend: cont address= "
                      << cont.address() << " "
                      << "" << handle_.address() << std::endl;
            std::cout
                << "MiddleTask FinalAwaiter await_suspend, resume continuation"
                << std::endl;
            handle_.promise().continuation_.resume();
        }

        void await_resume() noexcept {}
    };

    struct Awaiter {
        std::coroutine_handle<promise_type> handle_;

        bool await_ready() const noexcept { return false; }

        void await_suspend(std::coroutine_handle<> cont) {
            std::cout << "MiddleTask await_suspend: cont= " << cont.address()
                      << " "
                      << "handle= " << handle_.address() << std::endl;
            std::cout
                << "MiddleTask await_suspend:, do nothing, return to caller"
                << std::endl;
            handle_.promise().continuation_ = cont;
        }

        void await_resume() { spdlog::info("MiddleTask await_resume"); }
    };

    Awaiter operator co_await() {
        spdlog::info("MiddleTask co_await handle_ addr:{}", handle_.address());
        return Awaiter{.handle_ = handle_};
    }
};

struct LeafTask {
    struct FinalAwaiter;
    struct Awaiter;
    struct promise_type;
    std::coroutine_handle<promise_type> handle_;

    ~LeafTask() {
        if (handle_) {
            handle_.destroy();
            std::cout << "LeafTask  destructed" << std::endl;
        }
    }
    struct promise_type {
        std::coroutine_handle<> continue_;

        LeafTask get_return_object() noexcept {
            auto handle =
                std::coroutine_handle<promise_type>::from_promise(*this);
            std::cout << "LeafTask handle address: " << handle.address()
                      << std::endl;
            return LeafTask{.handle_ = handle};
        }

        std::suspend_always initial_suspend() const noexcept {
            spdlog::info("LeafTask initial_suspend");
            return {};
        }

        FinalAwaiter final_suspend() const noexcept {
            spdlog::info("LeafTask final_suspend");
            return FinalAwaiter{};
        }

        void unhandled_exception() noexcept {}

        void return_void() noexcept {}
    };

    struct FinalAwaiter {
        bool await_ready() const noexcept { return false; }

        // cont 是 FinalAwaiter 所在协程
        std::coroutine_handle<> await_suspend(
            std::coroutine_handle<promise_type> cont) noexcept {
            std::cout << "LeafTask FinalAwaiter await_suspend: cont= "
                      << cont.address() << std::endl;
            std::cout << "LeafTask FinalAwaiter await_suspend, resume caller"
                      << std::endl;
            auto p = cont.promise();
            spdlog::info("FinalAwaiter p.continue_:{}", p.continue_.address());
            return p.continue_;
        }

        void await_resume() noexcept {}
    };

    struct Awaiter {
        // 所在协程
        std::coroutine_handle<promise_type> handle_;

        bool await_ready() const noexcept { return false; }

        // cont: 调用方协程(即co_await所在协程). awaiter 所在协程已经被暂停
        void await_suspend(std::coroutine_handle<> cont) {
            handle_.promise().continue_ = cont;
            std::cout << "LeafTask await_suspend: con= " << cont.address()
                      << " "
                      << "handle " << handle_.address() << std::endl;
            auto c = handle_;
            auto f = [c]() mutable {
                std::cout << "LeafTask await_suspend, waker thread resume "
                             "coroutine after 10 s"
                          << std::endl;
                std::cout << "LeafTask await_suspend, waker thread pid: "
                          << std::this_thread::get_id() << " addr "
                          << c.address() << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds{5});
                c.resume();

                spdlog::info("finish");
            };
            std::thread(f).detach();
            std::cout << "LeafTask await_suspend, spawn waker thread, return "
                         "to caller"
                      << std::endl;
        }

        void await_resume() {
            std::cout << std::this_thread::get_id() << std::endl;
            spdlog::info("LeafTask await_resume");
        }
    };

    Awaiter operator co_await() {
        spdlog::info("LeafTask co_await handle_ addr:{}", handle_.address());
        return Awaiter{.handle_ = handle_};
    }
};

LeafTask bar() {
    std::cout << "befor bar" << std::endl;
    co_await std::suspend_never{};
    std::cout << "after bar, current pid: " << std::this_thread::get_id()
              << std::endl;
}

MiddleTask foo() {
    std::cout << "before foo" << std::endl;
    co_await bar();
    std::cout << "after foo" << std::endl;
}

TopTask boo() {
    // first call constructor TopTask coroutine
    // second if(initial_suspend)-->return caller where boo() func
    // when caller use handler call resume then excute  std::cout << "before
    // boo" << std::endl; then excute co_await foo()
    std::cout << "before boo" << std::endl;

    // foo() 标识创建一个task 挂起返回一个 MiddleTask 对象。
    // C++ 使用 MiddleTask::operator co_await() 来获取 Awaiter 对象。
    // 返回的 Awaiter 对象随后用于执行挂起和恢复逻辑。
    Tmp();
    auto obj = foo();
    spdlog::info("obj handle addr:{}", obj.handle_.address());
    // auto awaiter = obj.operator co_await();
    // --->if(awaiter.await_ready()){
    // awaiter.await_suspend(TopTask.handle_)
    // }
    co_await obj;
    std::cout << "after boo" << std::endl;
}

struct promise;

struct coroutine : std::coroutine_handle<promise> {
    using promise_type = ::promise;
};

struct promise {
    int val = 0;
    coroutine get_return_object() { return {coroutine::from_promise(*this)}; }
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_value(int i) { val = i; }
    void unhandled_exception() {}
};

struct S {
    int i;
    coroutine f() {
        i = 2;
        co_return i;
    }
};

void bad1() {
    coroutine h = S{0}.f();
    // S{0} destroyed
    h.resume();  // resumed coroutine executes std::cout << i, uses S::i after
                 // free
    h.destroy();
}

coroutine bad2() {
    S s{0};
    return s.f();  // returned coroutine can't be resumed without committing use
                   // after free
}
void bad3() {
    coroutine h = [i = 0]() -> coroutine  // a lambda that's also a coroutine
    {
        std::cout << i;
        co_return i;
    }();  // immediately invoked
    // lambda destroyed
    h.resume();  // uses (anonymous lambda type)::i after free
    h.destroy();
}

int main() {
    //    std::cout << std::this_thread::get_id() << std::endl;
    //    TopTask t = boo();
    //    std::cout << "TopTask is return pause: " << t.handle_.done() <<
    //    std::endl; t.handle_.resume(); std::cout << "TopTask is done: " <<
    //    t.handle_.done() << std::endl;
    //
    //    std::this_thread::sleep_for(std::chrono::seconds(15));
    //    bad3();
    task_func();
    return 0;
}
