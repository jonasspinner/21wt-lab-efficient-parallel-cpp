#include <chrono>
#include <barrier>
#include <vector>
#include <thread>
#include <iostream>
#include <cassert>
#include <iomanip>

#include "tbb/scalable_allocator.h"

#include "utils.h"

#include "lists/single_mutex_list.h"
#include "lists/atomic_marked_list.h"
#include "lists/node_mutex_list.h"

#include "instance_generation.h"

namespace benchmarks {
    template<class List>
    std::chrono::nanoseconds execute_instance(
            const std::vector<epcpp::Operation<int>> &setup,
            const std::vector<epcpp::Operation<int>> &queries,
            std::size_t num_threads) {
        List list;

        auto apply_op = [](auto &list, const auto &op) {
            switch (op.kind) {
                case epcpp::OperationKind::Insert: {
                    [[maybe_unused]] auto r = list.insert(op.value);
                    break;
                }
                case epcpp::OperationKind::Find: {
                    [[maybe_unused]] auto r = list.find(op.value);
                    break;
                }
                case epcpp::OperationKind::Erase: {
                    [[maybe_unused]] auto r = list.erase(op.value);
                    break;
                }
                default:
                    throw std::logic_error("");
            }
        };

        for (const auto &op: setup) {
            apply_op(list, op);
        }

        std::barrier ready(num_threads + 1);

        auto do_work = [&](std::size_t thread_idx) {
            ready.arrive_and_wait();

            auto start = queries.begin() + thread_idx * queries.size() / num_threads;
            auto end = queries.begin() + (thread_idx + 1) * queries.size() / num_threads;
            for (auto op = start; op != end; ++op) {
                apply_op(list, *op);
            }

            ready.arrive_and_wait();
        };

        std::vector<std::thread> threads;
        for (std::size_t thread_idx = 0; thread_idx < num_threads; ++thread_idx) {
            threads.emplace_back(do_work, thread_idx);
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

    template<class List,
            class Benchmark>
    void execute_benchmark(
            Benchmark benchmark,
            std::size_t log2_max_num_elements,
            std::size_t num_queries,
            std::size_t max_num_threads,
            std::size_t num_iterations
    ) {
        auto benchmark_name = Benchmark::name();
        auto list_name = List::name();

        std::cout << "benchmark_name,list_name,num_elements,num_queries,time,num_threads\n";

        for (std::size_t k = 0; k <= log2_max_num_elements; ++k) {
            std::size_t num_elements = 1 << k;

            auto[setup, queries] = benchmark.generate(num_elements, num_queries);
            for (std::size_t num_threads = 1; num_threads <= max_num_threads; ++num_threads) {
                for (std::size_t iteration = 0; iteration < num_iterations; ++iteration) {
                    auto time = benchmarks::execute_instance<List>(
                            setup, queries, num_threads);
                    std::cout
                            << std::setw(12) << benchmark_name << ", "
                            << std::setw(12) << list_name << ", "
                            << std::setw(12) << num_elements << ", "
                            << std::setw(12) << num_queries << ", "
                            << std::setw(16) << (double) time.count() << ", "
                            << std::setw(12) << num_threads
                            << std::endl;
                }
            }
        }
    }
}

int main() {
    using namespace benchmarks;
    using namespace epcpp;
    std::size_t log2_max_num_elements = 10;
    std::size_t num_queries = (1 << 10);
    std::size_t max_num_threads = 16;
    std::size_t num_iterations = 1;

    execute_benchmark<atomic_marked_list<int>>(successful_find_benchmark(), log2_max_num_elements, num_queries,
                                               max_num_threads, num_iterations);
    execute_benchmark<atomic_marked_list<int>>(unsuccessful_find_benchmark(), log2_max_num_elements, num_queries,
                                               max_num_threads, num_iterations);
    execute_benchmark<node_mutex_list<int>>(successful_find_benchmark(), log2_max_num_elements, num_queries,
                                            max_num_threads, num_iterations);
    execute_benchmark<node_mutex_list<int>>(unsuccessful_find_benchmark(), log2_max_num_elements, num_queries,
                                            max_num_threads, num_iterations);
    execute_benchmark<atomic_marked_list<int, tbb::scalable_allocator<int>>>(successful_find_benchmark(),
                                                                             log2_max_num_elements, num_queries,
                                                                             max_num_threads, num_iterations);
}
