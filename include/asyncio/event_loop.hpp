#pragma once
#include <queue>
#include <chrono>
#include <mutex>

#include "tiny_thread_pool.hpp"

#include "config.hpp"
#include "concepts.hpp"
#include "types.hpp"
#include "timer.hpp"
#include "export.hpp"


namespace kwa::asyncio {
    template<typename T>
    class FutureAwaiter;

    class ASYNCIO_EXPORT EventLoop {
        friend void Timer::cancel() noexcept;
        private:
            bool _stop { false };
            int _schedule_canceled_count { 0 };
            std::vector<std::shared_ptr<Timer>> _schedule {};
            std::queue<types::EventLoopHandle> _ready {};
            std::mutex _lock {};

            EventLoop();
            void _process_epoll(int timeout) noexcept;
            void _run_once() noexcept;
            inline TinyThreadPool& _get_pool() const noexcept {
                static TinyThreadPool pool { THREAD_POOL_SIZE };
                return pool;
            }
            static inline types::TimePoint _time() noexcept {
                return types::Clock::now();
            }
        public:
            [[nodiscard]] static EventLoop& get() noexcept;
            EventLoop(EventLoop&) = delete;
            EventLoop(EventLoop&&) = delete;
            EventLoop& operator=(EventLoop&) = delete;
            EventLoop& operator=(EventLoop&&) = delete;
            void call_soon(types::EventLoopHandle&& callback) noexcept;
            void call_soon_threadsafe(types::EventLoopHandle&& callback) noexcept;
            std::shared_ptr<Timer> call_at(types::TimePoint when, types::EventLoopHandle&& callback) noexcept;
            std::shared_ptr<Timer> call_later(std::chrono::milliseconds delay, types::EventLoopHandle&& callback) noexcept;
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
            [[nodiscard]] auto run_in_thread(F&& f, Args&&... args) noexcept
            -> FutureAwaiter<decltype(f(args...))> {
                return { _get_pool(), std::forward<F>(f), std::forward<Args>(args)... };
            }
    };
}
