#pragma once
#include <coroutine>
#include <utility>

#include <spdlog/spdlog.h>

#include "asyncio_ns.hpp"
#include "event_loop.hpp"
#include "concepts.hpp"
#include "handle.hpp"
#include "utils.hpp"


ASYNCIO_NS_BEGIN()

using namespace types;

template<typename R = void, typename E = void>
class Task {
    friend EventLoop;
private:
    struct BaseAwaiter;
    struct Promise; friend Promise;
    using CorounineHandle = std::coroutine_handle<Promise>;

    CorounineHandle _handle { nullptr };

    Task(Promise& p):
        _handle(CorounineHandle::from_promise(p))
    {
        EventLoop::get().call_soon(p);
    }

    inline void _check_valid() const noexcept {
        if (!valid()) utils::abort("Invalid Task");
    }
public:
    using result_type =  TaskResult<R, E>;
    using promise_type = Promise;
    using Callback = TaskCallback<R, E>;

    Task() = delete;
    Task(Task&) = delete;
    Task& operator=(Task&) = delete;

    Task(Task&& task):
        _handle(std::exchange(task._handle, nullptr))
    {
        //
    }

    Task& operator=(Task&& task) noexcept {
        _handle = std::exchange(task._handle, nullptr);
        return *this;
    }

    ~Task() {
        if (auto h = std::exchange(_handle, nullptr)) {
            if (h.done() || h.promise().canceled()) {
                h.destroy();
            } else {
                h.promise().suspend_at_final = false;
            }
        }
    }

    auto operator co_await() const & noexcept {
        struct Awaiter final: Task<R, E>::BaseAwaiter {
            decltype(auto) await_resume() const noexcept {
                return this->handle.promise().get_result();
            }
        };
        static_assert(concepts::Awaitable<Awaiter>);
        return Awaiter(_handle);
    }

    auto operator co_await() const && noexcept {
        struct Awaiter final: Task<R, E>::BaseAwaiter {
            auto await_resume() const noexcept {
                return std::move(this->handle.promise()).get_result();
            }
        };
        static_assert(concepts::Awaitable<Awaiter>);
        return Awaiter(_handle);
    }

    decltype(auto) result() & noexcept {
        _check_valid();
        return _handle.promise().get_result();
    }

    auto result() && noexcept {
        _check_valid();
        auto handle = std::exchange(_handle, nullptr);
        return std::move(handle.promise()).get_result();
    }

    inline void cancel() const noexcept {
        _check_valid();
        _handle.promise().cancel();
    }

    inline bool canceled() const noexcept {
        _check_valid();
        return _handle.promise().canceled();
    }

    inline void add_done_callback(Callback&& cb) const noexcept {
        _check_valid();
        _handle.promise().done_callbacks.push_back(std::move(cb));
    }

    inline bool done() const noexcept {
        _check_valid();
        return _handle.done();
    }

    inline Handle::ID id() const noexcept {
        _check_valid();
        return _handle.promise().id();
    }

    inline bool valid() const noexcept {
        return _handle != nullptr;
    }
};


template<typename R, typename E>
struct Task<R, E>::BaseAwaiter {
    CorounineHandle handle;

    BaseAwaiter(const CorounineHandle& h) noexcept: handle(h) {}

    bool await_ready(std::source_location loc = std::source_location::current()) const noexcept {
        if (handle) [[likely]] {
            handle.promise().loc = loc;
            return
                handle.done()
                || handle.promise().canceled();
        }
        return true;
    }

    template<typename P>
    requires concepts::Promise<P> || std::same_as<P, void>
    void await_suspend(std::coroutine_handle<P> h) const noexcept {
        handle.promise().set_parent(h.promise());
    }
};


template<typename R, typename E>
struct PromiseResult {
public:
    TaskResult<R, E> result {};
    bool result_ready { false };

    template<typename T>
    void return_value(T&& res) noexcept {
        if constexpr (std::is_void_v<R> && std::is_null_pointer_v<std::decay_t<T>>) {
            // nothing to do
        } else if constexpr (std::is_convertible_v<decltype(res), TaskResult<R, E>>) {
            result = std::forward<T>(res);
        } else if constexpr (std::is_convertible_v<decltype(res), E>) {
            result = std::unexpected<E>(std::forward<T>(res));
        } else {
            static_assert(false, "unexpected type");
        }
        result_ready = true;
    }
};


template<>
class PromiseResult<void, void> {
public:
    bool result_ready { false };

    void return_void() noexcept {
        result_ready = true;
    }
};


template<typename R, typename E>
struct Task<R, E>::Promise final: public PromiseResult<R, E>, public CoroHandle {
    bool suspend_at_final { true };
    std::vector<Callback> done_callbacks {};
    std::source_location loc {};
    bool has_unhandled_exception { false };

    Promise(std::source_location loc = std::source_location::current()): loc(loc) {}

    void schedule_callback() noexcept {
        if (!done_callbacks.empty()) {
            EventLoop::get().call_soon(
                [this]() {
                    for (auto& cb : this->done_callbacks) {
                        if constexpr (std::same_as<result_type, void>) {
                            cb();
                        } else cb(this->result);
                    }
                }
            );
        }
    }

    inline void run() noexcept override {
        CorounineHandle::from_promise(*this).resume();
    }

    inline const std::source_location& get_loc() const noexcept override {
        return loc;
    }

    void check_result() noexcept {
        if (canceled()) {
            traceback(0);
            utils::abort("Task Canceled");
        }
        if (!this->result_ready) {
            traceback(0);
            utils::abort("Task result not ready");
        }
    }

    decltype(auto) get_result() & noexcept {
        check_result();
        if constexpr (!std::is_void_v<result_type>) {
            return static_cast<result_type&>(this->result);
        }
    }

    result_type get_result() && noexcept {
        check_result();
        if constexpr (!std::is_void_v<result_type>) {
            return std::move(this->result);
        }
    }

    //////////////////////////////////////
    /// corounine related
    //////////////////////////////////////
    Task<R, E> get_return_object() noexcept {
        return { *this };
    }

    std::suspend_always initial_suspend() const noexcept {
        return {};
    }

    struct FinalAwaiter {
        const bool suspend;
        bool await_ready() noexcept { return !suspend; }
        template<typename P>
        requires concepts::Promise<P> || std::is_void_v<P>
        constexpr void await_suspend(std::coroutine_handle<P> handle) noexcept {}
        constexpr void await_resume() noexcept {}
    };
    FinalAwaiter final_suspend() noexcept {
        auto& loop = EventLoop::get();
        if (has_unhandled_exception) {
            traceback(0);
            loop.stop();
        } else if (!canceled()) {
            schedule_callback();
            if (loop.is_root(id())) {
                loop.stop();
            } else {
                try_resume_parent();
            }
        }
        return { suspend_at_final };
    }

    constexpr void unhandled_exception() noexcept {
        has_unhandled_exception = true;
        try {
            std::rethrow_exception(std::current_exception());
        } catch (const std::exception& e) {
            SPDLOG_ERROR("unhandled exception: {}", e.what());
        }
    }
};

static_assert(concepts::Task<Task<>>);
static_assert(concepts::Task<Task<void, int>>);
static_assert(concepts::Task<Task<int, int>>);
static_assert(concepts::Promise<Task<>::promise_type>);
static_assert(concepts::Promise<Task<void, int>::promise_type>);
static_assert(concepts::Promise<Task<int, int>::promise_type>);

ASYNCIO_NS_END
