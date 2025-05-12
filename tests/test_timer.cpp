#include <boost/ut.hpp>

#include "asyncio.hpp"
using namespace kwa;
using namespace boost::ut;
using namespace boost::ut::bdd;


int main() noexcept {
    "timer"_test = [] {
        given("call later") = [] {
            asyncio::run(
                [] -> asyncio::Task<> {
                    auto& loop = asyncio::EventLoop::get();
                    auto now = asyncio::Clock::now();
                    loop.call_later(std::chrono::milliseconds(500), [now] {
                        auto t = asyncio::Clock::now();
                        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t - now);
                        expect(duration.count() > 500-1 and duration.count() < 500+1) << duration.count();
                    });
                    co_await asyncio::sleep<600>();
                }()
            );
        };

        given("call at") = [] {
            asyncio::run(
                [] -> asyncio::Task<> {
                    auto& loop = asyncio::EventLoop::get();
                    auto now = asyncio::Clock::now();
                    loop.call_at(now + std::chrono::milliseconds(500), [now] {
                        auto t = asyncio::Clock::now();
                        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t - now);
                        expect(duration.count() > 500-1 and duration.count() < 500+1) << duration.count();
                    });
                    co_await asyncio::sleep<600>();
                }()
            );
        };
    };
}
