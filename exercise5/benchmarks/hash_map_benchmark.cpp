
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
#include "hash_map.h"
#include "other_hash_maps.h"
#include "instance_generation.h"

namespace benchmarks {
    template<class Map>
    std::chrono::nanoseconds execute_instance(
            const std::vector<epcpp::Operation<int>> &setup,
            const std::vector<epcpp::Operation<int>> &queries,
            std::size_t num_threads) {
        Map list(setup.size());

        auto apply_op = [](auto &list, const auto &op) -> int {
            switch (op.kind) {
                case epcpp::OperationKind::Insert: {
                    auto r = list.insert({op.value, op.value});
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

    template<class Map,
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

        auto benchmark_name = benchmark.name();
        auto map_name = Map::name();

        std::stringstream line;
        line << "benchmark_name,map_name,num_elements,num_queries,time,num_threads\n";
        std::cout << line.str();
        output << line.str();

        for (std::size_t k = 0; k <= log2_max_num_elements; k++) {
            std::size_t num_elements = 1 << k;
            for (std::size_t iteration = 0; iteration < num_iterations; ++iteration) {
                auto[setup, queries] = benchmark.generate(num_elements, num_queries, iteration);
                for (std::size_t num_threads = 1; num_threads <= max_num_threads; ++num_threads) {
                    auto time = benchmarks::execute_instance<Map>(
                            setup, queries, num_threads);
                    line.str("");
                    line
                            << "\"" << benchmark_name << "\", "
                            << "\"" << map_name << "\", "
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


    template<class Map>
    void execute_load_factor_benchmark(
            const std::string &output_filename,
            std::size_t capacity,
            std::size_t num_load_factors,
            float max_load_factor,
            std::size_t num_queries,
            std::size_t max_num_threads,
            std::size_t num_iterations
    ) {
        std::ofstream output(output_filename);

        auto benchmark_name = "load_factor";
        auto map_name = Map::name();

        auto benchmark = epcpp::successful_find_benchmark();

        std::stringstream line;
        line << "benchmark_name,map_name,capacity,num_elements,load_factor,num_queries,time,num_threads\n";
        std::cout << line.str();
        output << line.str();

        for (std::size_t i = 0; i < num_load_factors; i++) {
            std::size_t num_elements = i == 0 ? 1 : max_load_factor * i * capacity / (float) (num_load_factors - 1);
            float load_factor = (float) num_elements / (float) capacity;

            for (std::size_t iteration = 0; iteration < num_iterations; ++iteration) {
                auto[setup, queries] = benchmark.generate(num_elements, num_queries, iteration);
                for (std::size_t num_threads = 1; num_threads <= max_num_threads; ++num_threads) {
                    auto time = benchmarks::execute_instance<Map>(setup, queries, num_threads);
                    line.str("");
                    line
                            << "\"" << benchmark_name << "\", "
                            << "\"" << map_name << "\", "
                            << std::setw(12) << capacity << ", "
                            << std::setw(12) << num_elements << ", "
                            << std::setw(12) << load_factor << ", "
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
}

int main() {
    using namespace benchmarks;
    using namespace epcpp;
    std::size_t log2_max_num_elements = 20;
    std::size_t num_queries = (1 << 20);
    std::size_t max_num_threads = 16;
    std::size_t num_iterations = 10;

    using H01 = HashMap<std::hash<int>, ListBucket<int, int, single_mutex_list, std::equal_to<>, tbb::scalable_allocator<std::pair<const int, int>>, false>>;
    using H02 = HashMap<std::hash<int>, ListBucket<int, int, node_mutex_list, std::equal_to<>, tbb::scalable_allocator<std::pair<const int, int>>, false>>;
    using H03 = HashMap<std::hash<int>, ListBucket<int, int, atomic_marked_list, std::equal_to<>, tbb::scalable_allocator<std::pair<const int, int>>, false>>;
    using H04 = std_hash_map<int, int, std::hash<int>, std::equal_to<>, tbb::scalable_allocator<std::pair<const int, int>>>;
    using H05 = tbb_hash_map<int, int, std::hash<int>, std::equal_to<>, tbb::scalable_allocator<std::pair<const int, int>>>;

    execute_benchmark<H01>("../eval/H01_successful_find.csv", successful_find_benchmark(),
                           log2_max_num_elements, num_queries, max_num_threads, num_iterations);
    execute_benchmark<H02>("../eval/H02_successful_find.csv", successful_find_benchmark(),
                           log2_max_num_elements, num_queries, max_num_threads, num_iterations);
    execute_benchmark<H03>("../eval/H03_successful_find.csv", successful_find_benchmark(),
                           log2_max_num_elements, num_queries, max_num_threads, num_iterations);
    execute_benchmark<H04>("../eval/H04_successful_find.csv", successful_find_benchmark(),
                           log2_max_num_elements, num_queries, max_num_threads, num_iterations);
    execute_benchmark<H05>("../eval/H05_successful_find.csv", successful_find_benchmark(),
                           log2_max_num_elements, num_queries, max_num_threads, num_iterations);

    execute_benchmark<H01>("../eval/H01_unsuccessful_find.csv", unsuccessful_find_benchmark(),
                           log2_max_num_elements, num_queries, max_num_threads, num_iterations);
    execute_benchmark<H02>("../eval/H02_unsuccessful_find.csv", unsuccessful_find_benchmark(),
                           log2_max_num_elements, num_queries, max_num_threads, num_iterations);
    execute_benchmark<H03>("../eval/H03_unsuccessful_find.csv", unsuccessful_find_benchmark(),
                           log2_max_num_elements, num_queries, max_num_threads, num_iterations);
    execute_benchmark<H04>("../eval/H04_unsuccessful_find.csv", unsuccessful_find_benchmark(),
                           log2_max_num_elements, num_queries, max_num_threads, num_iterations);
    execute_benchmark<H05>("../eval/H05_unsuccessful_find.csv", unsuccessful_find_benchmark(),
                           log2_max_num_elements, num_queries, max_num_threads, num_iterations);

    execute_load_factor_benchmark<H01>("../eval/H01_load_factor.csv",
                                       1 << 10, 21, 64, 1 << 20, max_num_threads, num_iterations);
    execute_load_factor_benchmark<H02>("../eval/H02_load_factor.csv",
                                       1 << 10, 21, 64, 1 << 20, max_num_threads, num_iterations);
    execute_load_factor_benchmark<H03>("../eval/H03_load_factor.csv",
                                       1 << 10, 21, 64, 1 << 20, max_num_threads, num_iterations);
    execute_load_factor_benchmark<H04>("../eval/H04_load_factor.csv",
                                       1 << 10, 21, 64, 1 << 20, max_num_threads, num_iterations);
    execute_load_factor_benchmark<H05>("../eval/H05_load_factor.csv",
                                       1 << 10, 21, 64, 1 << 20, max_num_threads, num_iterations);
}
