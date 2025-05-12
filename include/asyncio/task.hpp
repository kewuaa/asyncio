#pragma once
#include <expected>
#include <coroutine>
#include <utility>

#include <spdlog/spdlog.h>

#include "asyncio_ns.hpp"
#include "event_loop.hpp"
#include "exception.hpp"
#include "concepts.hpp"
#include "handle.hpp"
#include "utils.hpp"


ASYNCIO_NS_BEGIN()

using namespace types;

template<typename R = void>
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
        exit_if(!valid(), "Invalid Task");
    }
public:
    using result_type =  Result<R>;
    using promise_type = Promise;
    using Callback = std::function<void(const result_type&)>;

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
            || _handle.promise().canceled()
            || _handle.promise().exception;
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
        if (!valid()) {
            return Error(InvalidTask());
        }
        return _handle.promise().get_result();
    }

    inline result_type result() && noexcept {
        if (!valid()) {
            return Error(InvalidTask());
        }
        return std::move(std::exchange(_handle, nullptr).promise()).get_result();
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


template<typename R>
struct PromiseResult {
protected:
    std::optional<R> result { std::nullopt };
public:
    template<typename T>
    requires std::is_same_v<R, typename std::remove_reference_t<T>>
    void return_value(T&& res) noexcept {
        result = std::forward<T>(res);
    }
};


template<>
class PromiseResult<void> {
public:
    void return_void() noexcept {
        //
    }
};


template<typename R>
struct Task<R>::Promise final: public PromiseResult<R>, public CoroHandle {
    bool suspend_at_final { true };
    std::vector<Callback> done_callbacks {};
    std::optional<Exception> exception { std::nullopt };

    void schedule_callback() noexcept {
        if (!done_callbacks.empty()) {
            EventLoop::get().call_soon(
                [this]() {
                    auto res = this->get_result();
                    for (auto& cb : this->done_callbacks) {
                        cb(res);
                    }
                }
            );
        }
    }

    template<typename E>
    requires std::is_base_of_v<Exception, typename std::remove_reference_t<E>>
    void set_exception(E&& e) noexcept {
        exception = std::forward<E>(e);
        schedule_callback();
        if (EventLoop::get().is_root(id())) {
            EventLoop::get().stop();
        } else {
            try_resume_parent();
        }
    }

    result_type get_result() && noexcept {
        if (canceled()) {
            exception = TaskCanceled();
        }
        auto e = std::exchange(exception, std::nullopt);
        if (e) {
            return Error(std::move(*e));
        }
        if constexpr (!std::is_void_v<R>) {
            if (!this->result) {
                return Error(TaskUnready());
            }
            return *std::exchange(this->result, std::nullopt);
        }
        return {};
    }

    result_type get_result() & noexcept {
        if (canceled()) {
            exception = TaskCanceled();
        }
        if (exception) {
            return Error(*exception);
        }
        if constexpr (!std::is_void_v<R>) {
            if (!this->result) {
                return Error(TaskUnready());
            }
            return *this->result;
        }
        return {};
    }

    inline void run() noexcept override {
        CorounineHandle::from_promise(*this).resume();
    }

    //////////////////////////////////////
    /// corounine related
    //////////////////////////////////////
    Task<R> get_return_object() noexcept {
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

    template<typename E>
    requires std::is_base_of_v<Exception, typename std::remove_reference_t<E>>
    std::suspend_always yield_value(E&& e) noexcept {
        set_exception(std::forward<E>(e));
        return {};
    }
};

static_assert(concepts::Task<Task<void>>, "Task<void> not satisfy the Task concept");
static_assert(concepts::Task<Task<int>>, "Task<int> not satisfy the Task concept");
static_assert(concepts::Promise<Task<void>::promise_type>, "Task<void>::promise_type not satisfy the Promise concept");
static_assert(concepts::Promise<Task<int>::promise_type>, "Task<int>::promise_type not satisfy the Promise concept");

ASYNCIO_NS_END
