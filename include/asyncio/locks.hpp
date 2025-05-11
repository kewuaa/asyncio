#pragma once
#include <queue>
#include <cassert>

#include "asyncio_ns.hpp"
#include "asyncio_export.hpp"
#include "asyncio/concepts.hpp"
#include "asyncio/handle.hpp"
#include "asyncio/event_loop.hpp"


ASYNCIO_NS_BEGIN()

class Lock;
class ASYNCIO_EXPORT Mutex {
    friend Lock;
public:
    Mutex() noexcept = default;
    Mutex(Mutex&) = delete;
    Mutex(Mutex&&) noexcept;
    Mutex& operator=(Mutex&) = delete;
    Mutex& operator=(Mutex&&) noexcept;
private:
    Handle* _owner { nullptr };
    std::queue<Handle*> _wait_list {};
};


class ASYNCIO_EXPORT Lock {
public:
    Lock(Mutex& mtx) noexcept;
    Lock(Lock&) = delete;
    Lock(Lock&&) = delete;
    Lock& operator=(Lock&) = delete;
    Lock& operator=(Lock&&) = delete;
    ~Lock() noexcept;

    constexpr bool await_ready() const noexcept { return false; }
    void await_resume() noexcept;

    template<typename P>
    requires concepts::Promise<P> || std::same_as<P, void>
    bool await_suspend(std::coroutine_handle<P> handle) noexcept {
        _handle = &handle.promise();
        if (!_mtx._owner) {
            _mtx._owner = _handle;
            return false;
        }
        if (_mtx._owner == _handle) {
            return false;
        }
        _mtx._wait_list.push(_handle);
        return true;
    }
private:
    Mutex& _mtx;
    Handle* _handle { nullptr };
};
static_assert(concepts::Awaitable<Lock>, "Lock is not awaitable");


template<typename T>
class Event {
    struct Waiter {
        Event& ev;
        Waiter(Waiter&) = delete;
        Waiter(Waiter&&) = delete;
        Waiter& operator=(Waiter&) = delete;
        Waiter& operator=(Waiter&&) = delete;
        Waiter(Event& ev) noexcept: ev(ev) {}

        constexpr bool await_ready() const noexcept { return false; }

        std::optional<T> await_resume() noexcept {
            return std::exchange(ev._msg, std::nullopt);
        }

        template<typename P>
        requires concepts::Promise<P> || std::same_as<P, void>
        void await_suspend(std::coroutine_handle<P> handle) noexcept {
            assert(ev._handle == nullptr);
            ev._handle = &handle.promise();
        }
    };
    static_assert(concepts::Awaitable<Waiter>, "Waiter is not awaitable");
public:
    Waiter wait() noexcept {
        return { *this };
    }

    void set() noexcept {
        assert(_handle != nullptr);
        EventLoop::get().call_soon(*std::exchange(_handle, nullptr));
    }

    void set(T&& msg) noexcept {
        _msg = std::make_optional(std::move(msg));
        set();
    }

    inline bool is_set() const noexcept { return _handle != nullptr; }
private:
    std::optional<T> _msg { std::nullopt };
    Handle* _handle { nullptr };
};


class ASYNCIO_EXPORT Condition {
    struct Waiter {
        Condition& cond;
        Waiter(Condition&) noexcept;
        Waiter(Waiter&) = delete;
        Waiter(Waiter&&) = delete;
        Waiter& operator=(Waiter&) = delete;
        Waiter& operator=(Waiter&&) = delete;
        constexpr bool await_ready() const noexcept { return false; }
        constexpr void await_resume() const noexcept {}

        template<typename P>
        requires concepts::Promise<P> || std::same_as<P, void>
        void await_suspend(std::coroutine_handle<P> handle) noexcept {
            cond._wait_list.push(&handle.promise());
        }
    };
    static_assert(concepts::Awaitable<Waiter>, "Waiter is not awaitable");
public:
    Condition() noexcept = default;
    Condition(Condition&) = delete;
    Condition(Condition&&) noexcept;
    Condition& operator=(Condition&) = delete;
    Condition& operator=(Condition&&) noexcept;
    Waiter wait() noexcept;
    void notify_one() noexcept;
    void notify_all() noexcept;
private:
    std::queue<Handle*> _wait_list {};
};

ASYNCIO_NS_END
