#pragma once
#include <queue>
#include <chrono>
#include <mutex>

#include "tiny_thread_pool.hpp"

#include "handle.hpp"
#include "concepts.hpp"
#include "types.hpp"
#include "timer.hpp"
#include "asyncio_ns.hpp"
#include "asyncio_config.hpp"
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
    void call_soon_threadsafe(EventLoopCallback&& callback) noexcept;
    std::shared_ptr<Timer> call_at(TimePoint when, EventLoopCallback&& callback) noexcept;
    std::shared_ptr<Timer> call_later(std::chrono::milliseconds delay, EventLoopCallback&& callback) noexcept;
    void stop() noexcept;
    void run() noexcept;

    inline bool is_root(Handle::ID id) const noexcept { return _root_id == id; }

    template<typename T>
    requires concepts::Task<T>
    [[nodiscard]] T::result_type run_until_complete(T&& task) noexcept {
        _root_id = task.id();
        run();
        return std::forward<T>(task).result();
    }

    template<typename F, typename... Args>
    requires requires(F f, Args... args) { f(args...); }
    [[nodiscard]] decltype(auto) run_in_thread(F&& f, Args&&... args) noexcept {
        using result_type = decltype(f(args...));
        return std::make_shared<FutureAwaiter<result_type>>(_get_pool(), std::forward<F>(f), std::forward<Args>(args)...);
    }
private:
    bool _stop { false };
    Handle::ID _root_id { 0 };
    std::vector<std::shared_ptr<Timer>> _schedule {};
    std::queue<EventLoopHandle> _ready {};
    std::mutex _lock {};

    EventLoop() noexcept;
    void _process_epoll(int timeout) noexcept;
    void _run_once() noexcept;
    void _cleanup() noexcept;
    inline TinyThreadPool& _get_pool() const noexcept {
        static TinyThreadPool pool { THREAD_POOL_SIZE };
        return pool;
    }
    static inline TimePoint _time() noexcept {
        return Clock::now();
    }
};

ASYNCIO_NS_END
