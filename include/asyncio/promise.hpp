#pragma once
#include <optional>
#include <cassert>

#include "concepts.hpp"
#include "asyncio_export.hpp"


namespace kwa::asyncio {
    template<typename R>
    class PromiseResult {
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
    class PromiseResult<void> {
        public:
            void return_void() noexcept {
                //
            }
    };

    class ASYNCIO_EXPORT BasePromise {
        private:
            mutable bool _canceled { false };
            BasePromise* _parent { nullptr };
        public:
            virtual void resume() noexcept = 0;

            inline void set_parent(BasePromise* parent) noexcept {
                assert(!_parent && "parent should be set once");
                _parent = parent;
            }

            inline void cancel() noexcept {
                _canceled = true;
            }

            bool canceled() const noexcept;

            void try_resume_parent() const noexcept;
    };

    static_assert(concepts::Cancelable<BasePromise>, "BasePromise not satisfy the Cancelable concept");
}
