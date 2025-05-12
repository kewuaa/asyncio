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
    struct Promise;
    friend Promise;
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

    inline void _check_result() const noexcept {
        if (canceled()) {
            utils::abort("Task Canceled");
        }
        if (!_handle.promise().result_ready) {
            utils::abort("Task result not ready");
        }
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

    bool await_ready() const noexcept {
        if (_handle) [[likely]] {
            return
            _handle.done()
            || _handle.promise().canceled();
        }
        return true;
    }

    template<typename P>
    requires concepts::Promise<P> || std::same_as<P, void>
    void await_suspend(std::coroutine_handle<P> handle) const noexcept {
        _handle.promise().set_parent(handle.promise());
    }

    result_type await_resume() const & noexcept {
        return result();
    }

    result_type await_resume() && noexcept {
        return std::move(*this)->result();
    }

    inline result_type result() const & noexcept {
        _check_valid();
        _check_result();
        if constexpr (!std::is_same_v<result_type, void>) {
            return _handle.promise().result;
        }
    }

    inline result_type result() && noexcept {
        _check_valid();
        _check_result();
        auto handle = std::exchange(_handle, nullptr);
        if constexpr (!std::is_same_v<result_type, void>) {
            return std::exchange((&handle.promise())->result, {});
        }
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
struct PromiseResult {
public:
    TaskResult<R, E> result {};
    bool result_ready { false };

    template<typename T>
    void return_value(T&& res) noexcept {
        if constexpr (std::is_same_v<std::decay_t<T>, R>) {
            result = std::forward<T>(res);
        } else if constexpr (std::is_same_v<std::decay_t<T>, E>) {
            result = std::unexpected<E>(std::forward<T>(res));
        } else if constexpr (std::is_same_v<std::decay_t<T>, std::nullptr_t>) {
            static_assert(std::is_void_v<R>, "only nullptr counld be return with [R = void]");
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
        if (canceled()) {
            return { suspend_at_final };
        }
        schedule_callback();
        if (EventLoop::get().is_root(id())) {
            EventLoop::get().stop();
        } else {
            try_resume_parent();
        }
        return { suspend_at_final };
    }

    constexpr void unhandled_exception() noexcept {
        return;
    }
};

static_assert(concepts::Task<Task<void>>, "Task<void> not satisfy the Task concept");
static_assert(concepts::Task<Task<int>>, "Task<int> not satisfy the Task concept");
static_assert(concepts::Promise<Task<void>::promise_type>, "Task<void>::promise_type not satisfy the Promise concept");
static_assert(concepts::Promise<Task<int>::promise_type>, "Task<int>::promise_type not satisfy the Promise concept");

ASYNCIO_NS_END
