#include <boost/ut.hpp>

#include "asyncio.hpp"
using namespace kwa;
using namespace boost::ut;
using namespace boost::ut::bdd;


template<size_t N>
asyncio::Task<> coro_depth_n(std::vector<int>& result) {
    result.push_back(N);
    if constexpr (N > 0) {
        co_await coro_depth_n<N - 1>(result);
        result.push_back(N * 10);
    }
}


asyncio::Task<int64_t> square(int64_t x) {
    co_return x * x;
}


int main() {
    "task"_test = [] {
        given("simple await") = [] {
            std::vector<int> result;
            auto res = asyncio::run(coro_depth_n<0>(result));
            expect(res.has_value());
            expect(result == std::vector<int>{0});
        };
        given("nest await") = [] {
            std::vector<int> result;
            auto res = asyncio::run(coro_depth_n<1>(result));
            expect(res.has_value());
            expect(result == std::vector<int>{1, 0, 10});
        };
        given("3 depth await") = [] {
            std::vector<int> result;
            auto res = asyncio::run(coro_depth_n<2>(result));
            expect(res.has_value());
            expect(result == std::vector<int>{2, 1, 0, 10, 20});
        };
        given("4 depth await") = [] {
            std::vector<int> result;
            auto res = asyncio::run(coro_depth_n<3>(result));
            expect(res.has_value());
            expect(result == std::vector<int>{ 3, 2, 1, 0, 10, 20, 30 });
        };
        given("5 depth await") = [] {
            std::vector<int> result;
            auto res = asyncio::run(coro_depth_n<4>(result));
            expect(res.has_value());
            expect(result == std::vector<int>{4, 3, 2, 1, 0, 10, 20, 30, 40});
        };
    };

    "Task<>"_test = [] {
        bool called { false };
        auto res = asyncio::run(
            [&] -> asyncio::Task<> {
                auto t = square(5);
                auto tt = std::move(t);
                auto res = co_await t;
                expect(!t.valid());
                expect(tt.valid());
                expect(!res.has_value());
                called = true;
            }()
        );
        expect(res.has_value());
        expect(called) << "task not run successfully";
    };

    "Task await result"_test = [] {
        given("square_sum 3, 4") = [] {
            auto square_sum = [](int x, int y) -> asyncio::Task<long> {
                auto tx = square(x);
                auto x2 = *(co_await tx);
                auto y2 = *(co_await square(y));
                co_return x2 + y2;
            };
            expect(*asyncio::run(square_sum(3, 4)) == 25);
        };
    };
}
