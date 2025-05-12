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
            asyncio::run(
                [&] -> asyncio::Task<> {
                    auto _ = task();
                    expect(order == 0); (order)++;
                    co_await asyncio::sleep<0>();
                    expect(order == 2);
                }()
            );
        };

        given("delay 1000") = [] {
            asyncio::run(
                [] -> asyncio::Task<> {
                    auto start = asyncio::types::Clock::now();
                    co_await asyncio::sleep<500>();
                    auto end = asyncio::types::Clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                    expect(duration > 499 and duration < 501);
                }()
            );
        };
    };
}
