#define _MIN_SCHEDULED_TIMER_HANDLES 100
#define _MIN_CANCELLED_TIMER_HANDLES_FRACTION 0.5
#include <algorithm>
#include <csignal>

#include <spdlog/spdlog.h>

#include "asyncio/config.hpp"
#include "asyncio/epoll.hpp"
#include "asyncio/event_loop.hpp"


namespace kwa::asyncio {
    static auto clock_resolution = std::chrono::milliseconds(
        1000 * types::Clock::period::num / types::Clock::period::den
    );

    static void signal_handle(int sig) noexcept {
        if (sig == SIGINT) {
            EventLoop::get().stop();
        }
    }

    EventLoop::EventLoop() {
        signal(SIGINT, signal_handle);
    }

    /// create event loop
    EventLoop& EventLoop::get() noexcept {
        static EventLoop loop;
        return loop;
    }

    /// process events of epoll
    void EventLoop::_process_epoll(int timeout) noexcept {
        auto& epoll = Epoll::get();
        epoll_event events[MAX_EVENTS_NUM];
        auto num = epoll.wait(events, MAX_EVENTS_NUM, timeout);
        for (int i = 0; i < num; i++) {
            auto ev = (Epoll::Event*)events[i].data.ptr;
            if ((events[i].events & EPOLLIN) && ev->reader) {
                _ready.push(ev->reader);
            }
            if ((events[i].events & EPOLLOUT) && ev->writer) {
                _ready.push(ev->writer);
            }
        }
    }

    /// run once loop
    void EventLoop::_run_once() noexcept {
        if (
            _schedule.size() > _MIN_SCHEDULED_TIMER_HANDLES
            && _schedule_canceled_count / (double)_schedule.size() > _MIN_CANCELLED_TIMER_HANDLES_FRACTION
        ) {
            decltype(_schedule) schedule;
            schedule.reserve(_schedule.size() - _schedule_canceled_count);
            for (auto& timer : _schedule) {
                if (!timer->_canceled) {
                    schedule.push_back(timer);
                }
            }
            std::make_heap(schedule.begin(), schedule.end());
            _schedule.swap(schedule);
            _schedule_canceled_count = 0;
        } else {
            while (!_schedule.empty() && _schedule.front()->_canceled) {
                std::pop_heap(_schedule.begin(), _schedule.end());
                _schedule.pop_back();
                _schedule_canceled_count -= 1;
            }
        }

        int timeout = -1;
        if (!_ready.empty() || _stop) {
            timeout = 0;
        } else if (!_schedule.empty()) {
            timeout = duration_cast<std::chrono::milliseconds>(
                _schedule[0]->_when - _time()
            ).count();
            if (timeout > MAX_SELECT_TIMEOUT) {
                timeout = MAX_SELECT_TIMEOUT;
            } else if (timeout < 0) {
                timeout = 0;
            }
        }
        _process_epoll(timeout);

        auto end_time = _time() + clock_resolution;
        while (!_schedule.empty()) {
            if (_schedule[0]->_when > end_time) {
                break;
            }
            _ready.push(
                [timer = _schedule[0]]() {
                    if (!timer->_canceled) {
                        timer->_callback();
                    }
                }
            );
            std::pop_heap(_schedule.begin(), _schedule.end());
            _schedule.pop_back();
        }

        while (!_ready.empty()) {
            if (_stop) {
                // if loop stop, clear ready queue
                do {
                    _ready.pop();
                } while (!_ready.empty());
            } else {
                _ready.front()();
                _ready.pop();
            }
        }
    }

    void EventLoop::call_soon(types::EventLoopHandle&& callback) noexcept {
        _ready.push(std::move(callback));
    }

    void EventLoop::call_soon_threadsafe(types::EventLoopHandle&& callback) noexcept {
        std::lock_guard<std::mutex> lock { _lock };
        _ready.push(std::move(callback));
    }

    std::shared_ptr<Timer> EventLoop::call_at(types::TimePoint when, types::EventLoopHandle&& callback) noexcept {
        auto timer = std::make_shared<Timer>(when, std::move(callback));
        _schedule.push_back(timer);
        std::push_heap(_schedule.begin(), _schedule.end());
        return timer;
    }

    std::shared_ptr<Timer> EventLoop::call_later(std::chrono::milliseconds delay, types::EventLoopHandle&& callback) noexcept {
        auto now = types::Clock::now();
        auto when = now + delay;
        return call_at(when, std::move(callback));
    }

    void EventLoop::stop() noexcept {
        _stop = true;
        spdlog::info("event loop stop");
    }

    /// run the event loop
    void EventLoop::run() noexcept {
        while (!_stop) {
            _run_once();
        }
    }
}
