#pragma once
#include <print>
#include <source_location>

#include <spdlog/spdlog.h>

#include "asyncio_ns.hpp"


ASYNCIO_NS_BEGIN(utils)

template<typename... Args>
inline void abort(
    spdlog::format_string_t<Args...> fmt,
    Args&&... args
) noexcept {
    spdlog::error(fmt, std::forward<Args>(args)...);
    if (errno != 0) {
        std::perror("error");
    }
    exit(EXIT_FAILURE);
}


inline void print_location(const std::source_location& loc, int depth) noexcept {
    if (depth == 0) {
        std::println("traceback below:");
    }
    std::println(
        "[{}] {} at {}:{}",
        depth,
        loc.function_name(),
        loc.file_name(),
        loc.line()
    );
}

ASYNCIO_NS_END
