#include <vector>
#include <random>
#include <iostream>
#include <chrono>
#include <new>
#include <utils/statistics.h>
#include <fstream>
#include <sstream>

#include "omp.h"
#include "utils/commandline.hpp"

template <class NumberType>
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

    template <class NumberType>
    NumberType single_threaded(const std::vector<NumberType> &values) {
        return std::accumulate(values.begin(), values.end(), NumberType{0});
    }

    template <class NumberType>
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

    template <class NumberType>
    NumberType false_cache_sharing(const std::vector<NumberType> &values) {
        int num_threads = omp_get_max_threads();
        std::vector<NumberType> sums(num_threads);
        std::size_t num_values = values.size();

        #pragma omp parallel default(none) shared(sums, values, num_values)
        {
            int id = omp_get_thread_num();
            #pragma omp for
            for (std::size_t i = 0; i < num_values; ++i) {
                sums[id] += values[i];
            }
        }
        return std::accumulate(sums.begin(), sums.end(), NumberType{0});
    }

    template <class NumberType>
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

    template <class NumberType>
    NumberType automatic_openmp(const std::vector<NumberType> &values) {
        NumberType sum{0};
        std::size_t num_values = values.size();

        #pragma omp parallel for default(none) shared(values, num_values) reduction(+:sum)
        for (std::size_t i = 0; i < num_values; ++i) {
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
    os << std::endl;
}

template <class NumberType>
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
    os << std::endl;
}

template<class F, class NumberType>
void time_function_call(std::ostream &file, std::string_view name, int num_threads, F f, const std::vector<NumberType> &values) {
    auto t0 = std::chrono::high_resolution_clock::now();
    auto result = f(values);
    auto t1 = std::chrono::high_resolution_clock::now();

    print_line(std::cout, name, values.size(), num_threads, result, t1 - t0);
    print_line(file, name, values.size(), num_threads, result, t1 - t0);
}


template <class NumberType>
void run_benchmark(std::size_t n, std::size_t seed,
                   const std::string& exp1_output_path, const std::string& exp2_output_path,
                   std::size_t num_steps, std::size_t num_iterations, bool skip_atomic_contention) {
    std::mt19937 gen(seed);

    auto all_values = generate_numbers<NumberType>(n, gen);

    std::ofstream exp1_output(exp1_output_path);
    print_header(exp1_output);
    print_header(std::cout);

    int max_threads = omp_get_max_threads();
    for (std::size_t step = 1; step <= num_steps; ++step) {
        auto num_values = n * step / num_steps;
        std::vector<NumberType> values(all_values.begin(), all_values.begin() + num_values);

        for (std::size_t iteration = 0; iteration < num_iterations; ++iteration) {
            time_function_call(exp1_output, "single_threaded", max_threads, benchmarks::single_threaded<NumberType>, values);
            if (!skip_atomic_contention)
                time_function_call(exp1_output, "atomic_contention", max_threads, benchmarks::atomic_contention<NumberType>, values);
            time_function_call(exp1_output, "false_cache_sharing", max_threads, benchmarks::false_cache_sharing<NumberType>, values);
            time_function_call(exp1_output, "fixed", max_threads, benchmarks::fixed<NumberType>, values);
            time_function_call(exp1_output, "automatic_openmp", max_threads, benchmarks::automatic_openmp<NumberType>, values);
        }
    }

    std::cout << "finished with exp1" << std::endl;

    std::ofstream exp2_output(exp2_output_path);
    print_header(exp2_output);
    print_header(std::cout);

    for (int num_threads = 1; num_threads <= max_threads; ++num_threads) {
        omp_set_num_threads(num_threads);

        for (std::size_t iteration = 0; iteration < num_iterations; ++iteration) {
            time_function_call(exp2_output, "single_threaded", num_threads, benchmarks::single_threaded<NumberType>, all_values);
            if (!skip_atomic_contention)
                time_function_call(exp2_output, "atomic_contention", num_threads, benchmarks::atomic_contention<NumberType>, all_values);
            time_function_call(exp2_output, "false_cache_sharing", num_threads, benchmarks::false_cache_sharing<NumberType>, all_values);
            time_function_call(exp2_output, "fixed", num_threads, benchmarks::fixed<NumberType>, all_values);
            time_function_call(exp2_output, "automatic_openmp", num_threads, benchmarks::automatic_openmp<NumberType>, all_values);
        }
    }

    std::cout << "finished with exp2" << std::endl;

}

int main(int argn, char** argc) {
    CommandLine c(argn, argc);

    int seed = c.intArg("-seed", 0);
    std::size_t n = c.intArg("-n", 1'000'000'000);
    std::size_t num_steps = c.intArg("-num-steps", 10);
    std::size_t num_iterations = c.intArg("-num-iterations", 10);
    std::string exp1_output_path = c.strArg("-exp1-output", "");
    std::string exp2_output_path = c.strArg("-exp2-output", "");
    std::string number_type = c.strArg("-number-type", "u64");
    bool skip_atomic_contention = c.boolArg("-skip-atomic-contention");
    c.report();

    if (exp1_output_path.empty()) {
        std::stringstream ss;
        ss << "access-pattern-exp1-" << n << "-" << number_type << ".csv";
        exp1_output_path = ss.str();
    }

    if (exp2_output_path.empty()) {
        std::stringstream ss;
        ss << "access-pattern-exp2-" << n << "-" << number_type << ".csv";
        exp2_output_path = ss.str();
    }

    if (number_type == "u8") {
        run_benchmark<uint8_t>(n, seed, exp1_output_path, exp2_output_path, num_steps, num_iterations, skip_atomic_contention);
    } else if (number_type == "u16") {
        run_benchmark<uint16_t>(n, seed, exp1_output_path, exp2_output_path, num_steps, num_iterations, skip_atomic_contention);
    } else if (number_type == "u32") {
        run_benchmark<uint32_t>(n, seed, exp1_output_path, exp2_output_path, num_steps, num_iterations, skip_atomic_contention);
    } else if (number_type == "u64") {
        run_benchmark<uint64_t>(n, seed, exp1_output_path, exp2_output_path, num_steps, num_iterations, skip_atomic_contention);
    } else if (number_type == "i8") {
        run_benchmark<int8_t>(n, seed, exp1_output_path, exp2_output_path, num_steps, num_iterations, skip_atomic_contention);
    } else if (number_type == "i16") {
        run_benchmark<int16_t>(n, seed, exp1_output_path, exp2_output_path, num_steps, num_iterations, skip_atomic_contention);
    } else if (number_type == "i32") {
        run_benchmark<int32_t>(n, seed, exp1_output_path, exp2_output_path, num_steps, num_iterations, skip_atomic_contention);
    } else if (number_type == "i64") {
        run_benchmark<int64_t>(n, seed, exp1_output_path, exp2_output_path, num_steps, num_iterations, skip_atomic_contention);
    } else {
        return 1;
    }


    return 0;
}