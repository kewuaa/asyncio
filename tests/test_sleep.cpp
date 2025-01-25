#include <boost/ut.hpp>

#include "asyncio.hpp"
using namespace kwa;
using namespace boost::ut;
using namespace boost::ut::bdd;


int main() {
    "sleep"_test = [] {
        given("delay 0") = [] {
            int order = 0;
            auto task = [&order] -> asyncio::Task<> {
                order++;
                co_return;
            };
            auto res = asyncio::run(
                [&] -> asyncio::Task<> {
                    auto _ = task();
                    expect(order == 0); (order)++;
                    co_await asyncio::sleep<0>();
                    expect(order == 2);
                }()
            );
            expect(res.has_value());
        };

        given("delay 1000") = [] {
            auto res = asyncio::run(
                [] -> asyncio::Task<> {
                    auto start = asyncio::types::Clock::now();
                    co_await asyncio::sleep<1000>();
                    auto end = asyncio::types::Clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                    expect(duration > 999 and duration < 1001);
                }()
            );
            expect(res.has_value());
        };
    };
}
