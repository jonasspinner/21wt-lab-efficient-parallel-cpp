#include <chrono>
#include <barrier>
#include <fstream>
#include <iomanip>

#include "instance_generation.h"
#include "bloom_filter.h"
#include "lists/atomic_marked_list.h"

namespace benchmarks {
    template<class Bucket>
    std::chrono::nanoseconds execute_instance(
            const std::vector<epcpp::Operation<int>> &setup,
            const std::vector<epcpp::Operation<int>> &queries,
            std::size_t num_threads) {
        Bucket bucket;

        auto apply_op = [](auto &bucket, const auto &op) -> int {
            auto hash = [](int key) {
                std::size_t x = key;
                x ^= x >> 33U;
                x *= UINT64_C(0xff51afd7ed558ccd);
                x ^= x >> 33U;
                //x *= UINT64_C(0xc4ceb9fe1a85ec53);
                //x ^= x >> 33U;
                return static_cast<size_t>(x);
            };

            switch (op.kind) {
                case epcpp::OperationKind::Insert: {
                    auto r = bucket.insert({op.value, op.value}, hash(op.value));
                    return r.second;
                }
                case epcpp::OperationKind::Find: {
                    auto r = bucket.find(op.value, hash(op.value));
                    return r == bucket.end();
                }
                case epcpp::OperationKind::Erase: {
                    auto r = bucket.erase(op.value, hash(op.value));
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
                    sum = sum + apply_op(bucket, *op);
                }
            }

            ready.arrive_and_wait();

            {
                auto start = queries.begin() + thread_idx * queries.size() / num_threads;
                auto end = queries.begin() + (thread_idx + 1) * queries.size() / num_threads;
                for (auto op = start; op != end; ++op) {
                    sum = sum + apply_op(bucket, *op);
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

    template<class BaseBucket, std::size_t K>
    void execute_benchmark(
            const std::string &output_filename,
            std::size_t num_success_probabilities,
            std::size_t log2_max_num_elements,
            std::size_t num_queries,
            std::size_t max_num_threads,
            std::size_t num_iterations
    ) {
        using Bucket = epcpp::BloomFilterAdapter<BaseBucket, K>;

        std::ofstream output(output_filename);

        auto benchmark_name = "find";
        auto bucket_name = Bucket::name();

        std::stringstream line;
        line
                << "benchmark_name,bucket_name,num_filters,find_success_probability,num_elements,num_queries,time,num_threads\n";
        std::cout << line.str();
        output << line.str();

        for (std::size_t i = 0; i < num_success_probabilities; ++i) {
            float find_success_probability = i == 0 ? 0 : (double)(1 << i) / (double)(1 << (num_success_probabilities - 1));
            //float find_success_probability = (float) i / (float) (num_success_probabilities - 1);

            auto benchmark = epcpp::find_benchmark(find_success_probability);

            for (std::size_t k = 0; k <= log2_max_num_elements; k++) {
                std::size_t num_elements = 1 << k;
                for (std::size_t iteration = 0; iteration < num_iterations; ++iteration) {
                    auto[setup, queries] = benchmark.generate(num_elements, num_queries, iteration);
                    for (std::size_t num_threads = 1; num_threads <= max_num_threads; ++num_threads) {
                        auto time = benchmarks::execute_instance<Bucket>(setup, queries, num_threads);
                        line.str("");
                        line
                                << "\"" << benchmark_name << "\", "
                                << "\"" << bucket_name << "\", "
                                << std::setw(12) << K << ", "
                                << std::setw(12) << find_success_probability << ", "
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
    }
}


int main() {
    using namespace benchmarks;
    using namespace epcpp;

    std::size_t num_success_probabilities = 11;
    std::size_t log2_max_num_elements = 10;
    std::size_t num_queries = 1 << 16;
    std::size_t max_num_threads = 4;
    std::size_t num_iterations = 10;

    using B01 = ListBucket<int, int, atomic_marked_list, std::equal_to<>, std::allocator<std::pair<const int, int>>, false>;

    execute_benchmark<B01, 0>("../eval/BF00_find.csv", num_success_probabilities,
                              log2_max_num_elements, num_queries, max_num_threads, num_iterations);
    execute_benchmark<B01, 1>("../eval/BF01_find.csv", num_success_probabilities,
                              log2_max_num_elements, num_queries, max_num_threads, num_iterations);
    execute_benchmark<B01, 2>("../eval/BF02_find.csv", num_success_probabilities,
                              log2_max_num_elements, num_queries, max_num_threads, num_iterations);
    execute_benchmark<B01, 3>("../eval/BF03_find.csv", num_success_probabilities,
                              log2_max_num_elements, num_queries, max_num_threads, num_iterations);
    execute_benchmark<B01, 4>("../eval/BF04_find.csv", num_success_probabilities,
                              log2_max_num_elements, num_queries, max_num_threads, num_iterations);
    execute_benchmark<B01, 5>("../eval/BF05_find.csv", num_success_probabilities,
                              log2_max_num_elements, num_queries, max_num_threads, num_iterations);
    execute_benchmark<B01, 6>("../eval/BF06_find.csv", num_success_probabilities,
                              log2_max_num_elements, num_queries, max_num_threads, num_iterations);
    execute_benchmark<B01, 7>("../eval/BF07_find.csv", num_success_probabilities,
                              log2_max_num_elements, num_queries, max_num_threads, num_iterations);
    execute_benchmark<B01, 8>("../eval/BF08_find.csv", num_success_probabilities,
                              log2_max_num_elements, num_queries, max_num_threads, num_iterations);
    execute_benchmark<B01, 9>("../eval/BF09_find.csv", num_success_probabilities,
                              log2_max_num_elements, num_queries, max_num_threads, num_iterations);
    execute_benchmark<B01, 10>("../eval/BF10_find.csv", num_success_probabilities,
                              log2_max_num_elements, num_queries, max_num_threads, num_iterations);

    return 0;
}
