#include <vector>
#include <random>
#include <iostream>
#include <chrono>
#include <new>
#include <utils/statistics.h>
#include <fstream>

#include "omp.h"
#include "utils/commandline.hpp"

using NumberType = u_int16_t;

std::vector<NumberType> generate_numbers(std::size_t n, std::mt19937 &gen) {
    std::uniform_int_distribution<NumberType> dist(NumberType{0},
                                                   std::numeric_limits<NumberType>::max());
    std::vector<NumberType> result;
    for (std::size_t i = 0; i < n; ++i) {
        result.push_back(dist(gen));
    }
    return result;
}

namespace benchmarks {

    NumberType single_threaded(const std::vector<NumberType> &values) {
        return std::accumulate(values.begin(), values.end(), NumberType{0});
    }

    NumberType atomic_contention(const std::vector<NumberType> &values) {
        NumberType sum{0};
        std::size_t num_values = values.size();
        #pragma omp parallel for default(none) shared(sum, values, num_values)
        for (std::size_t i = 0; i < num_values; ++i) {
            #pragma omp atomic
            sum += values[i];
        }
        return sum;
    }

    NumberType false_cache_sharing(const std::vector<NumberType> &values) {
        int num_threads = omp_get_max_threads();
        std::vector<NumberType> sums(num_threads);
        int num_values = (int)values.size();

#pragma omp parallel default(none) shared(sums, values, num_values)
        {
            int id = omp_get_thread_num();
#pragma omp for
            for (int i = 0; i < num_values; ++i) {
                sums[id] += values[i];
            }
        }
        return std::accumulate(sums.begin(), sums.end(), NumberType{0});
    }

    NumberType fixed(const std::vector<NumberType> &values) {
        struct alignas(64) Sum {
            NumberType value{0};
        };
        int num_threads = omp_get_max_threads();
        std::vector<Sum> sums(num_threads);
        std::size_t num_values = values.size();

        #pragma omp parallel default(none) shared(sums, values, num_values)
        {
            int id = omp_get_thread_num();
            #pragma omp for
            for (std::size_t i = 0; i < num_values; ++i) {
                sums[id].value += values[i];
            }
        }
        return std::accumulate(sums.begin(), sums.end(), NumberType{0}, [](NumberType a, const auto &b) { return a + b.value; });
    }

    NumberType automatic_openmp(const std::vector<NumberType> &values) {
        NumberType sum{0};
        int num_values = (int)values.size();

        #pragma omp parallel for default(none) shared(values, num_values) reduction(+:sum)
        for (int i = 0; i < num_values; ++i) {
            sum += values[i];
        }
        return sum;
    }
}

void print_header(std::ostream &os) {
    os.width(20);
    os << "name" << " ";
    os.width(10);
    os << "num_values" << " ";
    os.width(10);
    os << "num_threads" << " ";
    os.width(10);
    os << "result" << " ";
    os.width(10);
    os << "\"time (ms)\"" << " ";
    os << "\n";
}

void print_line(std::ostream &os, std::string_view name, std::size_t num_values, int num_threads, NumberType result, std::chrono::nanoseconds time) {
    os.width(20);
    os << name << " ";
    os.width(10);
    os << num_values << " ";
    os.width(10);
    os << num_threads << " ";
    os.width(10);
    os << result << " ";
    os.width(10);
    os << ((double)time.count() / 1'000'000.0) << " ";
    os << "\n";
}

template<class F>
void time_function_call(std::ostream &file, std::string_view name, int num_threads, F f, const std::vector<NumberType> &values) {
    auto t0 = std::chrono::high_resolution_clock::now();
    auto result = f(values);
    auto t1 = std::chrono::high_resolution_clock::now();

    print_line(std::cout, name, values.size(), num_threads, result, t1 - t0);
    print_line(file, name, values.size(), num_threads, result, t1 - t0);
}

int main(int argn, char** argc) {
    CommandLine c(argn, argc);

    int seed = c.intArg("-seed", 0);
    std::size_t n = c.intArg("-n", 1'000'000'000);
    std::size_t num_steps = c.intArg("-num-steps", 20);
    std::size_t num_iterations = c.intArg("-num-iterations", 20);
    std::string exp1_output_path = c.strArg("-exp1-output", "access-pattern-exp1-output.csv");
    std::string exp2_output_path = c.strArg("-exp2-output", "access-pattern-exp1-output.csv");
    c.report();

    std::mt19937 gen(seed);

    auto all_values = generate_numbers(n, gen);


    std::ofstream exp1_output(exp1_output_path);
    print_header(exp1_output);
    print_header(std::cout);

    int max_threads = omp_get_max_threads();
    for (std::size_t step = 1; step <= num_steps; ++step) {
        auto num_values = n * step / num_steps;
        std::vector<NumberType> values(all_values.begin(), all_values.begin() + num_values);

        for (std::size_t iteration = 0; iteration < num_iterations; ++iteration) {
            time_function_call(exp1_output, "single_threaded", 1, benchmarks::single_threaded, values);
            if (iteration == 0)
                time_function_call(exp1_output, "atomic_contention", max_threads, benchmarks::atomic_contention, values);
            time_function_call(exp1_output, "false_cache_sharing", max_threads, benchmarks::false_cache_sharing, values);
            time_function_call(exp1_output, "fixed", max_threads, benchmarks::fixed, values);
            time_function_call(exp1_output, "automatic_openmp", max_threads, benchmarks::automatic_openmp, values);
        }
    }


    std::ofstream exp2_output(exp2_output_path);
    print_header(exp2_output);
    print_header(std::cout);


    for (int num_threads = 1; num_threads <= max_threads; ++num_threads) {
        omp_set_num_threads(num_threads);

        for (std::size_t iteration = 0; iteration < num_iterations; ++iteration) {
            time_function_call(exp2_output, "single_threaded", num_threads, benchmarks::single_threaded, all_values);
            if (iteration == 0)
                time_function_call(exp2_output, "atomic_contention", num_threads, benchmarks::atomic_contention, all_values);
            time_function_call(exp2_output, "false_cache_sharing", num_threads, benchmarks::false_cache_sharing, all_values);
            time_function_call(exp2_output, "fixed", num_threads, benchmarks::fixed, all_values);
            time_function_call(exp2_output, "automatic_openmp", num_threads, benchmarks::automatic_openmp, all_values);
        }
    }

    return 0;
}