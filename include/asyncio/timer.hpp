#pragma once

#include "handle.hpp"
#include "types.hpp"
#include "concepts.hpp"
#include "asyncio_ns.hpp"
#include "asyncio_export.hpp"


ASYNCIO_NS_BEGIN()

using namespace types;

class EventLoop;
class ASYNCIO_EXPORT Timer: public Handle {
    friend EventLoop;
public:
    struct Compare;
    Timer(Timer&& timer);
    Timer(TimePoint when, EventLoopCallback&& callback);
    Timer& operator=(Timer&& timer) noexcept;
    void cancel() noexcept override;
    void run() noexcept override;
private:
    static size_t _canceled_count;
    TimePoint _when;
    EventLoopCallback _callback { nullptr };
};


struct Timer::Compare {
    inline bool operator()(const std::shared_ptr<Timer>& a, const std::shared_ptr<Timer>& b) {
        return a->_when > b->_when;
    }
};

static_assert(concepts::Cancelable<Timer>, "Timer not satisfy the Cancelable concept");

ASYNCIO_NS_END
