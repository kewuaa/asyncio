#include <boost/ut.hpp>

#include "asyncio.hpp"
using namespace kwa;
using namespace boost::ut;
using namespace boost::ut::bdd;


int main() noexcept {
    "timer"_test = [] {
        given("call later") = [] {
            auto res = asyncio::run(
                [] -> asyncio::Task<> {
                    auto& loop = asyncio::EventLoop::get();
                    auto now = asyncio::types::Clock::now();
                    loop.call_later(std::chrono::milliseconds(500), [now] {
                        auto t = asyncio::types::Clock::now();
                        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t - now);
                        expect(duration.count() > 499 and duration.count() < 501);
                    });
                    co_await asyncio::sleep<600>();
                }()
            );
            expect(res.has_value());
        };

        given("call at") = [] {
            auto res = asyncio::run(
                [] -> asyncio::Task<> {
                    auto& loop = asyncio::EventLoop::get();
                    auto now = asyncio::types::Clock::now();
                    loop.call_at(now + std::chrono::milliseconds(500), [now] {
                        auto t = asyncio::types::Clock::now();
                        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t - now);
                        expect(duration.count() > 499 && duration.count() < 501);
                    });
                    co_await asyncio::sleep<600>();
                }()
            );
            expect(res.has_value());
        };
    };
}
