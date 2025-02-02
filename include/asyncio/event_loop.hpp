#pragma once
#include <queue>
#include <chrono>
#include <mutex>

#include "tiny_thread_pool.hpp"

#include "concepts.hpp"
#include "types.hpp"
#include "timer.hpp"
#include "asyncio_config.hpp"
#include "asyncio_export.hpp"


namespace kwa::asyncio {
    using namespace types;

    template<typename T>
    class FutureAwaiter;

    class ASYNCIO_EXPORT EventLoop {
        friend void Timer::cancel() noexcept;
        private:
            bool _stop { false };
            int _schedule_canceled_count { 0 };
            std::vector<std::shared_ptr<Timer>> _schedule {};
            std::queue<EventLoopHandle> _ready {};
            std::mutex _lock {};

            EventLoop();
            void _process_epoll(int timeout) noexcept;
            void _run_once() noexcept;
            inline TinyThreadPool& _get_pool() const noexcept {
                static TinyThreadPool pool { THREAD_POOL_SIZE };
                return pool;
            }
            static inline TimePoint _time() noexcept {
                return Clock::now();
            }
        public:
            [[nodiscard]] static EventLoop& get() noexcept;
            EventLoop(EventLoop&) = delete;
            EventLoop(EventLoop&&) = delete;
            EventLoop& operator=(EventLoop&) = delete;
            EventLoop& operator=(EventLoop&&) = delete;
            void call_soon(EventLoopHandle&& callback) noexcept;
            void call_soon_threadsafe(EventLoopHandle&& callback) noexcept;
            std::shared_ptr<Timer> call_at(TimePoint when, EventLoopHandle&& callback) noexcept;
            std::shared_ptr<Timer> call_later(std::chrono::milliseconds delay, EventLoopHandle&& callback) noexcept;
            void stop() noexcept;
            void run() noexcept;

            template<typename T>
            requires concepts::Task<T>
            && requires(T task) {
                { task._set_root() } -> std::same_as<void>;
            }
            [[nodiscard]] T::result_type run_until_complete(T&& task) noexcept {
                task._set_root();
                _stop = false;
                run();
                return std::forward<T>(task).result();
            }

            template<typename F, typename... Args>
            requires requires(F f, Args... args) { f(args...); }
            [[nodiscard]] decltype(auto) run_in_thread(F&& f, Args&&... args) noexcept {
                using result_type = decltype(f(args...));
                return std::make_shared<FutureAwaiter<result_type>>(_get_pool(), std::forward<F>(f), std::forward<Args>(args)...);
            }
    };
}
