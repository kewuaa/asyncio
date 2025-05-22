#define _MIN_SCHEDULED_TIMER_HANDLES 100
#define _MIN_CANCELLED_TIMER_HANDLES_FRACTION 0.5
#include <algorithm>
#include <csignal>

#include <spdlog/spdlog.h>

#include "asyncio_config.hpp"
#include "asyncio/epoll.hpp"
#include "asyncio/event_loop.hpp"


ASYNCIO_NS_BEGIN()

static auto clock_resolution = std::chrono::milliseconds(
    1000 * Clock::period::num / Clock::period::den
);

static void signal_handle(int sig) noexcept {
    if (sig == SIGINT) {
        EventLoop::get().stop();
        SPDLOG_WARN("receive SIGINT signal");
    }
}

EventLoop::EventLoop() noexcept {
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
        if ((events[i].events & EPOLLIN) && ev->reader.has_value()) {
            _ready.push(*ev->reader);
        }
        if ((events[i].events & EPOLLOUT) && ev->writer.has_value()) {
            _ready.push(*ev->writer);
        }
    }
}

/// run once loop
void EventLoop::_run_once() noexcept {
    if (
        _schedule.size() > _MIN_SCHEDULED_TIMER_HANDLES
        && Timer::_canceled_count / (double)_schedule.size() > _MIN_CANCELLED_TIMER_HANDLES_FRACTION
    ) {
        decltype(_schedule) schedule;
        schedule.reserve(_schedule.size() - Timer::_canceled_count);
        for (auto& timer : _schedule) {
            if (!timer->canceled()) {
                schedule.push_back(timer);
            }
        }
        std::make_heap(schedule.begin(), schedule.end(), Timer::Compare());
        _schedule.swap(schedule);
        Timer::_canceled_count = 0;
    } else {
        while (!_schedule.empty() && _schedule.front()->canceled()) {
            std::pop_heap(_schedule.begin(), _schedule.end(), Timer::Compare());
            _schedule.pop_back();
            Timer::_canceled_count -= 1;
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
        _ready.push({ _schedule[0]->id(), [t = _schedule[0]] { t->run(); } });
        std::pop_heap(_schedule.begin(), _schedule.end(), Timer::Compare());
        _schedule.pop_back();
    }

    std::vector<Handle::ID> canceled {};
    if (!_stop && !_ready.empty()) {
        auto size = _ready.size();
        for (size_t i = 0; !_stop && i < size; ++i) {
            if (auto id = _ready.front().id; id == 0 || !Handle::canceled(id)) {
                _ready.front().cb();
            } else if (Handle::canceled(id)) {
                canceled.push_back(id);
            }
            _ready.pop();
        }
    }
    for (auto id : canceled) {
        Handle::_canceled_handles.erase(id);
    }
}

void EventLoop::_cleanup() noexcept {
    _stop = false;
    _root_id = 0;
    while (!_ready.empty()) {
        _ready.pop();
    }
    Handle::_canceled_handles.clear();
}

void EventLoop::call_soon(EventLoopCallback&& callback) noexcept {
    _ready.push({ 0, std::move(callback) });
}

void EventLoop::call_soon(Handle& handle) noexcept {
    _ready.push({ handle.id(), [h = &handle] { h->run(); } });
}

std::shared_ptr<Timer> EventLoop::call_at(TimePoint when, EventLoopCallback&& callback) noexcept {
    auto timer = std::make_shared<Timer>(when, std::move(callback));
    _schedule.push_back(timer);
    std::push_heap(_schedule.begin(), _schedule.end(), Timer::Compare());
    return timer;
}

std::shared_ptr<Timer> EventLoop::call_later(std::chrono::milliseconds delay, EventLoopCallback&& callback) noexcept {
    auto now = Clock::now();
    auto when = now + delay;
    return call_at(when, std::move(callback));
}

void EventLoop::stop() noexcept {
    _stop = true;
    SPDLOG_INFO("event loop stop");
}

/// run the event loop
void EventLoop::run() noexcept {
    while (!_stop) {
        _run_once();
    }
    _cleanup();
}

ASYNCIO_NS_END
