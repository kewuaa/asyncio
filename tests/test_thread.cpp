#include <boost/ut.hpp>

#include "asyncio.hpp"
using namespace kwa;
using namespace boost::ut;
using namespace boost::ut::bdd;


int thread_task() {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    return 1000;
}


int main() {
    "thread"_test = [] {
        auto res = asyncio::run(
            [] -> asyncio::Task<> {
                auto& loop = asyncio::EventLoop::get();
                std::vector<decltype(loop.run_in_thread(thread_task))> futs;
                for (int i = 0; i < 3; i++) {
                    futs.push_back(loop.run_in_thread(thread_task));
                }
                co_await asyncio::sleep<500>();
                for (auto fut : futs) {
                    auto res = co_await *fut;
                    expect(res == 1000);
                }
            }()
        );
    };
}
