#include <spdlog/spdlog.h>

#include "asyncio/types.hpp"
#include "asyncio/timer.hpp"
#include "asyncio/event_loop.hpp"


namespace kwa::asyncio {
    using namespace types;

    uint32_t Timer::_ID = 0;

    Timer::Timer(TimePoint when, EventLoopHandle&& callback):
        _id(++_ID),
        _when(when),
        _callback(std::move(callback))
    {
        SPDLOG_INFO("Timer {} created", _id);
    }

    Timer::Timer(Timer&& timer):
        _id(std::exchange(timer._id, 0)),
        _when(timer._when),
        _canceled(timer._canceled),
        _callback(std::exchange(timer._callback, nullptr))
    {
        //
    }

    Timer& Timer::operator=(Timer&& timer) noexcept {
        _id = std::exchange(timer._id, 0);
        _when = timer._when;
        _canceled = timer._canceled;
        _callback = std::exchange(timer._callback, nullptr);
        return *this;
    }

    void Timer::cancel() noexcept {
        _canceled = true;
        EventLoop::get()._schedule_canceled_count++;
        SPDLOG_INFO("Timer {} canceled", _id);
    }

    bool operator<(const Timer& t1, const Timer& t2) {
        return t1._when > t2._when;
    }
}
