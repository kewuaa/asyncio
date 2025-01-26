#pragma once
#include <cassert>

#include "concepts.hpp"
#include "export.hpp"


namespace kwa::asyncio {
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
