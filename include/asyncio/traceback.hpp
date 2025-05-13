#pragma once
#include <source_location>

#include <spdlog/fmt/fmt.h>

#include "asyncio_ns.hpp"
#include "asyncio/concepts.hpp"
#include "asyncio/utils.hpp"


ASYNCIO_NS_BEGIN()

struct TracebackAwaiter {
    constexpr bool await_ready(
        std::source_location loc = std::source_location::current()
    ) noexcept {
        _loc = loc;
        return false;
    }
    constexpr void await_resume() const noexcept {}
    template<typename P>
    requires concepts::Promise<P>
    bool await_suspend(std::coroutine_handle<P> handle) const noexcept {
        utils::print_location(_loc, 0);
        handle.promise().traceback(1);
        return false;
    }
private:
    std::source_location _loc {};
};


[[nodiscard]] constexpr TracebackAwaiter traceback() noexcept { return {}; }

ASYNCIO_NS_END
