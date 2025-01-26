#pragma once
#include <spdlog/spdlog.h>


namespace kwa::asyncio {
    template<typename... Args>
    inline void exit_if(
        bool cond,
        spdlog::format_string_t<Args...> fmt,
        Args&&... args
    ) noexcept {
        if (cond) {
            spdlog::error(fmt, std::forward<Args>(args)...);
            if (errno != 0) {
                std::perror("error");
            }
            exit(EXIT_FAILURE);
        }
    }
}
