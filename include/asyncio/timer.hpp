#pragma once
#include "types.hpp"
#include "concepts.hpp"
#include "asyncio_export.hpp"


namespace kwa::asyncio {
    using namespace types;

    class EventLoop;
    class ASYNCIO_EXPORT Timer {
        friend EventLoop;
        friend bool operator<(const Timer& t1, const Timer& t2);
        private:
            static uint32_t _ID;
            uint32_t _id { 0 };
            TimePoint _when;
            bool _canceled { false };
            EventLoopHandle _callback;
        public:
            struct Compare;
            Timer() = delete;
            Timer(Timer&) = delete;
            Timer& operator=(Timer&) = delete;
            Timer(Timer&& timer);
            Timer(TimePoint when, EventLoopHandle&& callback);
            Timer& operator=(Timer&& timer) noexcept;
            void cancel() noexcept;
            inline bool canceled() const noexcept { return _canceled; }
    };

    struct Timer::Compare {
        inline bool operator()(const std::shared_ptr<Timer>& a, const std::shared_ptr<Timer>& b) {
            return a->_when > b->_when;
        }
    };

    static_assert(concepts::Cancelable<Timer>, "Timer not satisfy the Cancelable concept");
}
