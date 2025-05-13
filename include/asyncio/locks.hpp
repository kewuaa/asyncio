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
struct EventMessage {
    using type = std::optional<T>;
    type msg { std::nullopt };
};
template<>
struct EventMessage<void> {
    using type = void;
};

template<typename T = void>
class Event: private EventMessage<T> {
    struct Awaiter {
        Event& ev;
        Awaiter(Awaiter&) = delete;
        Awaiter(Awaiter&&) = delete;
        Awaiter& operator=(Awaiter&) = delete;
        Awaiter& operator=(Awaiter&&) = delete;
        Awaiter(Event& ev) noexcept: ev(ev) {}

        constexpr bool await_ready() const noexcept { return false; }

        EventMessage<T>::type await_resume() noexcept {
            if constexpr (!std::is_void_v<T>) {
                return std::exchange(ev.msg, std::nullopt);
            }
        }

        template<typename P>
        requires concepts::Promise<P> || std::same_as<P, void>
        void await_suspend(std::coroutine_handle<P> handle) noexcept {
            assert(ev._handle == nullptr);
            ev._handle = &handle.promise();
        }
    };
    static_assert(concepts::Awaitable<Awaiter>, "Awaiter not satisfy Awaitable concept");
public:
    Event() noexcept: EventMessage<T>() {}
    Event(Event&) = delete;
    Event(Event&&) = delete;
    Event& operator=(Event&) = delete;
    Event& operator=(Event&&) = delete;

    Awaiter wait() noexcept {
        return { *this };
    }

    template<typename... Message>
    void set(Message&&... msg) noexcept {
        assert(_handle != nullptr);
        if constexpr (!std::is_void_v<T>) {
            static_assert(sizeof...(Message) < 2, "too much message");
            if constexpr (sizeof...(Message) == 1) {
                this->msg = std::make_optional(std::forward<Message>(msg)...);
            }
        } else static_assert(sizeof...(Message) == 0, "not support messsage");
        EventLoop::get().call_soon(*std::exchange(_handle, nullptr));
    }

    inline bool is_set() const noexcept { return _handle == nullptr; }
private:
    Handle* _handle { nullptr };
};


class ASYNCIO_EXPORT Condition {
    struct Awaiter {
        Condition& cond;
        Awaiter(Condition&) noexcept;
        Awaiter(Awaiter&) = delete;
        Awaiter(Awaiter&&) = delete;
        Awaiter& operator=(Awaiter&) = delete;
        Awaiter& operator=(Awaiter&&) = delete;
        constexpr bool await_ready() const noexcept { return false; }
        constexpr void await_resume() const noexcept {}

        template<typename P>
        requires concepts::Promise<P> || std::same_as<P, void>
        void await_suspend(std::coroutine_handle<P> handle) noexcept {
            cond._wait_list.push(&handle.promise());
        }
    };
    static_assert(concepts::Awaitable<Awaiter>, "AWaiter not satisify awaitable concept");
public:
    Condition() noexcept = default;
    Condition(Condition&) = delete;
    Condition(Condition&&) noexcept;
    Condition& operator=(Condition&) = delete;
    Condition& operator=(Condition&&) noexcept;
    Awaiter wait() noexcept;
    void notify_one() noexcept;
    void notify_all() noexcept;
private:
    std::queue<Handle*> _wait_list {};
};

ASYNCIO_NS_END
