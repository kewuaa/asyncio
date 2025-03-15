#include <spdlog/spdlog.h>

#include "asyncio/types.hpp"
#include "asyncio/timer.hpp"


ASYNCIO_NS_BEGIN()

using namespace types;

size_t Timer::_canceled_count = 0;

Timer::Timer(TimePoint when, EventLoopCallback&& callback):
    Handle(),
    _when(when),
    _callback(std::move(callback))
{
}

Timer::Timer(Timer&& timer):
    Handle(std::move(timer)),
    _when(timer._when),
    _callback(std::exchange(timer._callback, nullptr))
{
    //
}

Timer& Timer::operator=(Timer&& timer) noexcept {
    _when = timer._when;
    _callback = std::exchange(timer._callback, nullptr);
    return *this;
}

void Timer::cancel() noexcept {
    _canceled_count++;
    Handle::cancel();
}

void Timer::run() noexcept {
    _callback();
}

ASYNCIO_NS_END
