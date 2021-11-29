#include <vector>
#include <random>
#include <iostream>
#include <chrono>
#include <new>
#include <utils/statistics.h>

#include "omp.h"

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
        #pragma omp parallel for default(none) shared(sum, values)
        for (int value : values) {
            #pragma omp atomic
            sum += value;
        }
        return sum;
    }

    NumberType false_cache_sharing(const std::vector<NumberType> &values) {
        int num_threads = omp_get_max_threads();
        std::vector<NumberType> sums(num_threads);

        #pragma omp parallel default(none) shared(sums, values)
        {
            int id = omp_get_thread_num();
            #pragma omp for
            for (int value : values) {
                sums[id] += value;
            }
        }
        return std::accumulate(sums.begin(), sums.end(), NumberType{0});
    }

    NumberType false_cache_sharing2(const std::vector<NumberType> &values) {
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

        #pragma omp parallel default(none) shared(sums, values)
        {
            int id = omp_get_thread_num();
            #pragma omp for
            for (int value : values) {
                sums[id].value += value;
            }
        }
        return std::accumulate(sums.begin(), sums.end(), NumberType{0}, [](NumberType a, const auto &b) { return a + b.value; });
    }

    NumberType fixed2(const std::vector<NumberType> &values) {
        struct alignas(64) Sum {
            NumberType value{0};
        };
        int num_threads = omp_get_max_threads();

        std::vector<Sum> sums(num_threads);

        auto* sum_ptr = sums.data();
        auto* values_ptr = values.data();
        int num_values = (int)values.size();

#pragma omp parallel default(none) shared(sum_ptr, values_ptr, num_values)
        {
            int id = omp_get_thread_num();
#pragma omp for
            for (int i = 0; i < num_values; ++i) {
                sum_ptr[id].value += values_ptr[i];
            }
        }
        return std::accumulate(sums.begin(), sums.end(), NumberType{0}, [](NumberType a, const auto &b) { return a + b.value; });
    }

    NumberType fixed3(const std::vector<NumberType> &values) {
        struct alignas(64) Sum {
            NumberType value{0};
        };
        int num_threads = omp_get_max_threads();
        std::vector<Sum> sums(num_threads);
        int num_values = (int)values.size();

#pragma omp parallel default(none) shared(sums, values, num_values)
        {
            int id = omp_get_thread_num();
#pragma omp for
            for (int i = 0; i < num_values; ++i) {
                sums[id].value += values[i];
            }
        }
        return std::accumulate(sums.begin(), sums.end(), NumberType{0}, [](NumberType a, const auto &b) { return a + b.value; });
    }

    NumberType automatic_openmp(const std::vector<NumberType> &values) {
        NumberType sum{0};
        int num_values = (int)values.size();

#pragma omp parallel for default(none) shared(values, num_values) reduction(+:sum) schedule(static)
        for (int i = 0; i < num_values; ++i) {
            sum += values[i];
        }
        return sum;
    }
}

template<class F, class ...Args>
void time_function_call(std::string_view name, F f, Args&& ... args) {

    std::vector<double> times;
    for (int i = 0; i < 10; ++i) {
        auto t0 = std::chrono::high_resolution_clock::now();
        [[maybe_unused]] auto result = f(std::forward<Args>(args)...);
        auto t1 = std::chrono::high_resolution_clock::now();
        times.push_back(static_cast<double>((t1 - t0).count()) / 1'000'000.);
    }
    std::cout.width(20);
    std::cout << name << " ";
    //std::cout << "sum=";
    //std::cout.width(10);
    //std::cout << result << " ";
    std::cout.width(10);
    std::cout << statistics::mean(times) << " ms ";
    std::cout.width(10);
    std::cout << statistics::standard_deviation(times) << " ms\n";
}

int main() {
    std::mt19937 gen;
    auto values = generate_numbers(1'000'000'000, gen);

    time_function_call("single_threaded", benchmarks::single_threaded, values);
    //time_function_call(benchmarks::atomic_contention, values);
    time_function_call("false_cache_sharing", benchmarks::false_cache_sharing, values);
    time_function_call("false_cache_sharing2", benchmarks::false_cache_sharing2, values);
    time_function_call("fixed", benchmarks::fixed, values);
    time_function_call("fixed2", benchmarks::fixed2, values);
    time_function_call("fixed3", benchmarks::fixed3, values);
    time_function_call("automatic_openmp", benchmarks::automatic_openmp, values);

    return 0;
}