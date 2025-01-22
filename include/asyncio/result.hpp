#pragma once
#include <optional>


namespace kwa::asyncio {
    template<typename R>
    class Result {
        protected:
            std::optional<R> result { std::nullopt };
        public:
            template<typename T>
            requires std::is_same_v<R, typename std::remove_reference_t<T>>
            void return_value(T&& res) noexcept {
                result = std::forward<T>(res);
            }
    };

    template<>
    class Result<void> {
        public:
            void return_void() noexcept {
                //
            }
    };
}
