
#include <vector>
#include <thread>
#include <iostream>
#include <mutex>
#include <algorithm>

#include "implementation/concurrent_container.hpp"

#include "implementation/mutex_std_queue.hpp"
#include "utils/misc.h"

template<class C>
void test_container(C &container, size_t num_threads, size_t num_values_per_thread) {
    std::mutex m;

    std::vector<std::thread> threads;

    std::vector<int> popped_values;

    auto produce = [&]() {
        for (size_t i = 0; i < num_values_per_thread; ++i) {
            int e = (int) i + 1;
            container.push(e);
        }
    };

    auto consume = [&]() {
        for (size_t i = 0; i < num_values_per_thread; ++i) {
            auto e = container.pop();
            {
                std::scoped_lock l(m);
                popped_values.push_back(e);
            }
        }
    };


    threads.reserve(num_threads * 2);

    auto t0 = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < num_threads; ++i) {
        threads.emplace_back(produce);
        threads.emplace_back(consume);
    }

    for (auto &t: threads) {
        t.join();
    }

    auto t1 = std::chrono::high_resolution_clock::now();

    std::cout << num_threads << " ";
    std::cout << popped_values.size() << " ";
    std::cout << utils::to_ms(t1 - t0) << " ms" << std::endl;
}


int main() {
    ConcurrentContainer<int> container(1 << 10);
    test_container(container, 10, 10000);
    return 0;
}
