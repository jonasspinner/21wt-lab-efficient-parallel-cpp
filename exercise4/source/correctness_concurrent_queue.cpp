
#include <vector>
#include <thread>
#include <iostream>
#include <mutex>
#include <algorithm>

#include "implementation/concurrent_queue.hpp"
#include "implementation/concurrent_queue_cas_element.hpp"

#include "implementation/mutex_std_queue.hpp"
#include "utils/misc.h"
#include "implementation/concurrent_queue.hpp"

template<class Q>
void test_queue(Q &queue, size_t num_threads, size_t num_values_per_thread, size_t print_last_elements = 0) {
    if (!queue.empty()) {
        throw std::runtime_error("[error]   queue is non-empty");
    }

    std::mutex m;

    std::vector<std::thread> threads;

    std::vector<int> popped_values;

    std::atomic<unsigned> num_pushed = 0;

    auto produce = [&](int thread_id) {
        for (size_t i = 0; i < num_values_per_thread; ++i) {
            int e = (int) i + 1;
            queue.push(e);
            num_pushed++;
            if (i >= num_values_per_thread - print_last_elements) {
                std::cout << "producer " << thread_id << " " << i << " " << num_pushed << std::endl;
            }
        }
    };

    std::atomic<unsigned> num_popped = 0;

    auto consume = [&](int thread_id) {
        for (size_t i = 0; i < num_values_per_thread; ++i) {
            auto e = queue.pop();
            num_popped++;
            {
                std::scoped_lock l(m);
                popped_values.push_back(e);
            }
            if (i >= num_values_per_thread - print_last_elements) {
                std::cout << "consumer " << thread_id << " " << i << " " << num_popped << std::endl;
            }
        }
    };


    threads.reserve(num_threads * 2);

    auto t0 = std::chrono::high_resolution_clock::now();

    int thread_num = 0;
    for (size_t i = 0; i < num_threads; ++i) {
        threads.emplace_back(produce, thread_num++);
        threads.emplace_back(consume, thread_num++);
    }

    for (auto &t: threads) {
        t.join();
    }

    auto t1 = std::chrono::high_resolution_clock::now();

    if (!queue.empty()) {
        throw std::runtime_error("[failed]   queue is non-empty");
    }

    std::cout << "[info]     ";
    std::cout << num_threads << " ";
    std::cout << popped_values.size() << " ";
    std::cout << utils::to_ms(t1 - t0) << " ms" << std::endl;

    std::cout << "[passed]   passed many ops" << std::endl;
}

template<class Q>
void test_bounded_capacity() {
    {
        size_t capacity = 0;
        Q queue(capacity);
        if (queue.try_push(1)) {
            throw std::runtime_error("[failed]   pushed with zero capacity");
        }
        std::cout << "[passed]   capacity == 0" << std::endl;
    }
    {
        size_t capacity = 10;
        Q queue(capacity);
        for (size_t i = 0; i < capacity; ++i) {
            if (!queue.try_push(1)) {
                queue.print_state();
                throw std::runtime_error("[failed]   not enough capacity");
            }
        }
        if (queue.try_push(1)) {
            throw std::runtime_error("[failed]   more capacity than specified");
        }
        std::cout << "[passed]   capacity > 0" << std::endl;
    }
}


int main() {
    {
        ConcurrentQueue<int> concurrentQueue(1 << 10);
        test_queue(concurrentQueue, 1, 10000);

        MutexStdQueue<int> mutexStdQueue(1 << 10);
        test_queue(mutexStdQueue, 1, 10000);
    }

    {
        ConcurrentQueue<int> concurrentQueue(1 << 10);
        test_queue(concurrentQueue, 10, 10000);

        MutexStdQueue<int> mutexStdQueue(1 << 10);
        test_queue(mutexStdQueue, 10, 10000);
    }


    test_bounded_capacity<ConcurrentQueue<int>>();
    test_bounded_capacity<ConcurrentQueueCASElement<int>>();


    /*
    {
        ConcurrentQueueCASElement<int> mutexStdQueue(1 << 10);
        test_queue(mutexStdQueue, 1, 10000, 100);
    }
     */

    return 0;
}
