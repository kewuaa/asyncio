#pragma once
#include <queue>
#include <chrono>

#include "handle.hpp"
#include "concepts.hpp"
#include "types.hpp"
#include "timer.hpp"
#include "asyncio_ns.hpp"
#include "asyncio_export.hpp"


ASYNCIO_NS_BEGIN()

using namespace types;

template<typename T>
class FutureAwaiter;

class ASYNCIO_EXPORT EventLoop {
public:
    [[nodiscard]] static EventLoop& get() noexcept;
    EventLoop(EventLoop&) = delete;
    EventLoop(EventLoop&&) = delete;
    EventLoop& operator=(EventLoop&) = delete;
    EventLoop& operator=(EventLoop&&) = delete;
    void call_soon(EventLoopCallback&& callback) noexcept;
    void call_soon(Handle& handle) noexcept;
    std::shared_ptr<Timer> call_at(TimePoint when, EventLoopCallback&& callback) noexcept;
    std::shared_ptr<Timer> call_later(std::chrono::milliseconds delay, EventLoopCallback&& callback) noexcept;
    void stop() noexcept;
    void run() noexcept;

    inline bool is_root(Handle::ID id) const noexcept { return _root_id == id; }

    template<typename T>
    requires concepts::Task<T>
    [[nodiscard]] decltype(auto) run_until_complete(T&& task) noexcept {
        _root_id = task.id();
        run();
        return std::forward<T>(task).result();
    }

    template<typename Pool, typename F, typename... Args>
    [[nodiscard]] inline decltype(auto) run_in_thread(Pool& pool, F&& f, Args&&... args) noexcept {
        using result_type = decltype(f(args...));
        return std::make_shared<FutureAwaiter<result_type>>(pool, std::forward<F>(f), std::forward<Args>(args)...);
    }
private:
    bool _stop { false };
    Handle::ID _root_id { 0 };
    std::vector<std::shared_ptr<Timer>> _schedule {};
    std::queue<EventLoopHandle> _ready {};

    EventLoop() noexcept;
    void _process_epoll(int timeout) noexcept;
    void _run_once() noexcept;
    void _cleanup() noexcept;
    static inline TimePoint _time() noexcept {
        return Clock::now();
    }
};

ASYNCIO_NS_END
