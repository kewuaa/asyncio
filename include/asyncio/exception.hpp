#pragma once
#include <format>
#include <expected>

#include "asyncio_export.hpp"


namespace kwa::asyncio {
    class ASYNCIO_EXPORT Exception {
        friend std::ostream& operator<<(std::ostream& s, const Exception& e) noexcept;
        private:
            std::string _msg {};
        public:
            Exception() = delete;
            Exception(Exception&);
            Exception(Exception&&);
            Exception(int error_code);
            Exception(const char* msg);
            Exception& operator=(Exception&) noexcept;
            Exception& operator=(Exception&&) noexcept;

            template<typename S>
            requires std::same_as<S, std::string>
            Exception(S&& s): _msg(std::forward<S>(s)) {
                //
            }

            template<typename... Args>
            Exception(std::format_string<Args...> fmt, Args&&... args):
                _msg(std::format(fmt, std::forward<Args>(args)...))
            {
                //
            }

            inline const char* message() const noexcept {
                return _msg.c_str();
            }
    };
}
