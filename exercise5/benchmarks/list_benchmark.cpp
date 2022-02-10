#include <chrono>
#include <barrier>
#include <vector>
#include <thread>
#include <iostream>
#include <cassert>
#include <iomanip>
#include <fstream>

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

        auto apply_op = [](auto &list, const auto &op) -> int {
            switch (op.kind) {
                case epcpp::OperationKind::Insert: {
                    auto r = list.insert(op.value);
                    return r.second;
                }
                case epcpp::OperationKind::Find: {
                    auto r = list.find(op.value);
                    return r == list.end();
                }
                case epcpp::OperationKind::Erase: {
                    auto r = list.erase(op.value);
                    return r;
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
            volatile int sum = 0;
            {
                auto start = setup.begin() + thread_idx * setup.size() / num_threads;
                auto end = setup.begin() + (thread_idx + 1) * setup.size() / num_threads;
                for (auto op = start; op != end; ++op) {
                    sum = sum + apply_op(list, *op);
                }
            }

            ready.arrive_and_wait();
            {
                auto start = queries.begin() + thread_idx * queries.size() / num_threads;
                auto end = queries.begin() + (thread_idx + 1) * queries.size() / num_threads;
                for (auto op = start; op != end; ++op) {
                    sum = sum + apply_op(list, *op);
                }
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
            const std::string &output_filename,
            Benchmark benchmark,
            std::size_t log2_max_num_elements,
            std::size_t num_queries,
            std::size_t max_num_threads,
            std::size_t num_iterations
    ) {
        std::ofstream output(output_filename);

        auto benchmark_name = Benchmark::name();
        auto list_name = List::name();

        std::stringstream line;
        line << "benchmark_name,list_name,num_elements,num_queries,time,num_threads\n";
        std::cout << line.str();
        output << line.str();

        for (std::size_t k = 0; k <= log2_max_num_elements; ++k) {
            std::size_t num_elements = 1 << k;

            for (std::size_t iteration = 0; iteration < num_iterations; ++iteration) {
                auto[setup, queries] = benchmark.generate(num_elements, num_queries, iteration);
                for (std::size_t num_threads = 1; num_threads <= max_num_threads; ++num_threads) {
                    auto time = benchmarks::execute_instance<List>(
                            setup, queries, num_threads);
                    line.str("");
                    line
                            << "\"" << benchmark_name << "\", "
                            << "\"" << list_name << "\", "
                            << std::setw(12) << num_elements << ", "
                            << std::setw(12) << num_queries << ", "
                            << std::setw(16) << (double) time.count() << ", "
                            << std::setw(12) << num_threads
                            << std::endl;
                    std::cout << line.str();
                    output << line.str();
                }
            }
        }
    }

    template<class List>
    void execute_find_and_modifiy_benchmark(
            const std::string &output_filename,
            std::size_t num_elements,
            float successful_find_probability,
            std::size_t num_modification_probabilities,
            std::size_t num_queries,
            std::size_t max_num_threads,
            std::size_t num_iterations
    ) {
        std::ofstream output(output_filename);

        auto benchmark_name = "find_and_modifiy";
        auto list_name = List::name();

        std::stringstream line;
        line
                << "benchmark_name,list_name,num_elements,num_queries,successful_find_probability,modification_probability,time,num_threads\n";
        std::cout << line.str();
        output << line.str();

        for (std::size_t i = 0; i < num_modification_probabilities; ++i) {
            float modification_probability = i == 0 ? 0 : (double)(1 << i) / (double)(1 << (num_modification_probabilities - 1));

            auto benchmark = epcpp::find_and_modifiy_benchmark(successful_find_probability, modification_probability);

            for (std::size_t iteration = 0; iteration < num_iterations; ++iteration) {
                auto[setup, queries] = benchmark.generate(num_elements, num_queries, iteration);
                for (std::size_t num_threads = 1; num_threads <= max_num_threads; ++num_threads) {
                    auto time = benchmarks::execute_instance<List>(
                            setup, queries, num_threads);
                    line.str("");
                    line
                            << "\"" << benchmark_name << "\", "
                            << "\"" << list_name << "\", "
                            << std::setw(12) << num_elements << ", "
                            << std::setw(12) << num_queries << ", "
                            << std::setw(12) << successful_find_probability << ", "
                            << std::setw(12) << modification_probability << ", "
                            << std::setw(16) << (double) time.count() << ", "
                            << std::setw(12) << num_threads
                            << std::endl;
                    std::cout << line.str();
                    output << line.str();
                }
            }
        }
    }
}

int main() {
    using namespace benchmarks;
    using namespace epcpp;
    std::size_t log2_max_num_elements = 8;
    std::size_t num_queries = (1 << 20);
    std::size_t max_num_threads = 16;
    std::size_t num_iterations = 10;

    using L01 = single_mutex_list<int, tbb::scalable_allocator<int>>;
    using L02 = node_mutex_list<int, tbb::scalable_allocator<int>>;
    using L03 = atomic_marked_list<int, tbb::scalable_allocator<int>>;

    execute_benchmark<L01>("../eval/L01_successful_find.csv", successful_find_benchmark(),
                           log2_max_num_elements, num_queries, max_num_threads, num_iterations);
    execute_benchmark<L02>("../eval/L02_successful_find.csv", successful_find_benchmark(),
                           log2_max_num_elements, num_queries, max_num_threads, num_iterations);
    execute_benchmark<L03>("../eval/L03_successful_find.csv", successful_find_benchmark(),
                           log2_max_num_elements, num_queries, max_num_threads, num_iterations);

    execute_benchmark<L01>("../eval/L01_unsuccessful_find.csv", unsuccessful_find_benchmark(),
                           log2_max_num_elements, num_queries, max_num_threads, num_iterations);
    execute_benchmark<L02>("../eval/L02_unsuccessful_find.csv", unsuccessful_find_benchmark(),
                           log2_max_num_elements, num_queries, max_num_threads, num_iterations);
    execute_benchmark<L03>("../eval/L03_unsuccessful_find.csv", unsuccessful_find_benchmark(),
                           log2_max_num_elements, num_queries, max_num_threads, num_iterations);

    execute_find_and_modifiy_benchmark<L01>("../eval/L01_find_and_modifiy.csv",
                                            1 << 8, 0.1, 11, 1 << 16, max_num_threads, num_iterations);
    execute_find_and_modifiy_benchmark<L02>("../eval/L02_find_and_modifiy.csv",
                                            1 << 8, 0.1, 11, 1 << 16, max_num_threads, num_iterations);
    execute_find_and_modifiy_benchmark<L03>("../eval/L03_find_and_modifiy.csv",
                                            1 << 8, 0.1, 11, 1 << 16, max_num_threads, num_iterations);
}
