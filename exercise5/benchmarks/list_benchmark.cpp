#include <chrono>
#include <barrier>
#include <vector>
#include <thread>
#include <iostream>
#include <cassert>
#include <iomanip>

#include "utils.h"

#include "lists/single_mutex_list.h"
#include "lists/_leaking_atomic_list.h"
#include "lists/atomic_marked_list.h"
#include "lists/node_mutex_list.h"


namespace benchmarks::successful_find {
    template<template<class> class List>
    std::chrono::nanoseconds run(std::size_t num_elements, std::size_t num_threads, std::size_t num_iterations) {
        List<int> list;

        for (std::size_t i = 0; i < num_elements; ++i) {
            auto[h, inserted] = list.insert((int) i);
            assert(inserted);
        }

        std::barrier ready(num_threads + 1);

        std::atomic<int> total_sum;

        auto do_work = [&]() {
            ready.arrive_and_wait();

            int sum = 0;
            for (std::size_t iteration = 0; iteration < num_iterations; ++iteration) {
                for (std::size_t i = 0; i < num_elements; ++i) {
                    auto h = list.find((int) i);
                    assert(h != list.end());
                    sum += *h;
                }
            }

            total_sum.fetch_add(sum);

            ready.arrive_and_wait();
        };

        std::vector<std::thread> threads;
        for (std::size_t thread_idx = 0; thread_idx < num_threads; ++thread_idx) {
            threads.emplace_back(do_work);
        }

        ready.arrive_and_wait();
        auto t0 = std::chrono::high_resolution_clock::now();

        ready.arrive_and_wait();
        auto t1 = std::chrono::high_resolution_clock::now();


        for (auto &thread: threads) {
            thread.join();
        }

        return t1 - t0;
    }
}


namespace benchmarks::unsuccessful_find {
    template<template<class> class List>
    std::chrono::nanoseconds run(std::size_t num_elements, std::size_t num_threads, std::size_t num_iterations) {
        List<int> list;

        for (std::size_t i = 0; i < num_elements; ++i) {
            auto[h, inserted] = list.insert((int) i);
            assert(inserted);
        }

        std::barrier ready(num_threads + 1);

        std::atomic<int> total_sum;

        auto do_work = [&]() {
            ready.arrive_and_wait();

            int sum = 0;
            for (std::size_t iteration = 0; iteration < num_iterations; ++iteration) {
                for (std::size_t i = num_elements; i < 2 * num_elements; ++i) {
                    auto h = list.find((int) i);
                    assert(h == list.end());
                    sum += (h == list.end());
                }
            }

            total_sum.fetch_add(sum);

            ready.arrive_and_wait();
        };

        std::vector<std::thread> threads;
        for (std::size_t thread_idx = 0; thread_idx < num_threads; ++thread_idx) {
            threads.emplace_back(do_work);
        }

        ready.arrive_and_wait();
        auto t0 = std::chrono::high_resolution_clock::now();

        ready.arrive_and_wait();
        auto t1 = std::chrono::high_resolution_clock::now();


        for (auto &thread: threads) {
            thread.join();
        }

        return t1 - t0;
    }
}

int main() {
    std::string name = "node_mutex_list";
    //std::size_t num_threads = 4;
    std::size_t num_iterations = 100;

    for (std::size_t num_threads = 1; num_threads <= 16; ++num_threads) {
        for (std::size_t k = 0; k <= 12; ++k) {
            std::size_t num_elements = 1 << k;

            auto time = benchmarks::unsuccessful_find::run<epcpp::atomic_marked_list>(num_elements, num_threads,
                                                                                     num_iterations);
            std::cout
                    << std::setw(12) << name << ", "
                    << std::setw(12) << num_elements << ", "
                    << std::setw(16) << (double)time.count() / (double)num_iterations << ", "
                    << std::setw(12) << num_threads
                    << std::endl;
        }
    }
}
