#include <benchmark/benchmark.h>

#include <tbb/scalable_allocator.h>

#include "instance_generation.h"
#include "hash_map.h"
#include "other_hash_maps.h"


template<class ...T>
//using Allocator = std::allocator<T...>;
using Allocator = tbb::scalable_allocator<T...>;


template<template<class, class, class, class, class> class Map>
static void BM_Find(benchmark::State &state) {
    using ConcreteMap = Map<int, int, std::hash<int>, std::equal_to<>, Allocator<std::pair<const int, int>>>;

    static std::minstd_rand gen{0};

    ConcreteMap map(state.range(0));
    for (int i = 0; i < state.range(0); ++i) {
        auto result = map.insert({gen(), gen()});
    }

    for (auto _: state) {
        benchmark::DoNotOptimize(map.find(gen()));
    }
}

BENCHMARK(BM_Find<epcpp::hash_map>)->ArgsProduct({benchmark::CreateRange(256, 1 << 20, 64)});
BENCHMARK(BM_Find<epcpp::tbb_hash_map>)->ArgsProduct({benchmark::CreateRange(256, 1 << 20, 64)});
BENCHMARK(BM_Find<epcpp::std_hash_map>)->ArgsProduct({benchmark::CreateRange(256, 1 << 20, 64)});


template<template<class, class, class, class, class> class Map>
static void BM_BatchInsertWithClear(benchmark::State &state) {
    using ConcreteMap = Map<int, int, std::hash<int>, std::equal_to<>, Allocator<std::pair<const int, int>>>;

    static std::minstd_rand gen{0};

    ConcreteMap map(state.range(1));

    for (auto _: state) {
        state.PauseTiming();
        map.clear();
        for (int i = 0; i < state.range(1); ++i) {
            map.insert({gen(), gen()});
        }
        state.ResumeTiming();
        for (int i = 0; i < state.range(0); ++i) {
            auto result = map.insert({gen(), 0});
            benchmark::DoNotOptimize(result);
        }
    }

    state.SetComplexityN(state.range(0));
}

//BENCHMARK(BM_BatchInsertWithClear<epcpp::hash_map>)->ArgsProduct({benchmark::CreateRange(1, 64, 4), benchmark::CreateRange(256, 1 << 10, 256)})->Complexity();
//BENCHMARK(BM_BatchInsertWithClear<epcpp::tbb_hash_map>)->ArgsProduct({benchmark::CreateRange(1, 64, 4), benchmark::CreateRange(256, 1 << 10, 256)})->Complexity();
//BENCHMARK(BM_BatchInsertWithClear<epcpp::std_hash_map>)->ArgsProduct({benchmark::CreateRange(1, 64, 4), benchmark::CreateRange(256, 1 << 10, 256)})->Complexity();

template<template<class, class, class, class, class> class Map>
static void BM_InsertErase(benchmark::State &state) {
    using ConcreteMap = Map<int, int, std::hash<int>, std::equal_to<>, Allocator<std::pair<const int, int>>>;

    static std::minstd_rand gen{0};

    auto mask = std::numeric_limits<int>::min();

    ConcreteMap map(state.range(0));
    for (int i = 0; i < state.range(0); ++i) {
        map.insert({gen() & ~mask, gen()});
    }

    for (auto _: state) {
        int x = gen() | mask;
        benchmark::DoNotOptimize(map.insert({x, 0}));
        benchmark::DoNotOptimize(map.erase(x));
    }

    state.SetComplexityN(state.range(0));
}

BENCHMARK(BM_InsertErase<epcpp::hash_map>)->ArgsProduct({benchmark::CreateRange(256, 1 << 20, 64)})->Complexity();
BENCHMARK(BM_InsertErase<epcpp::std_hash_map>)->ArgsProduct({benchmark::CreateRange(256, 1 << 20, 64)})->Complexity();


template<template<class, class, class, class, class> class Map>
static void BM_BatchInsert(benchmark::State &state) {
    using ConcreteMap = Map<int, int, std::hash<int>, std::equal_to<>, Allocator<std::pair<const int, int>>>;

    static std::minstd_rand gen{0};

    auto mask = std::numeric_limits<int>::min();

    ConcreteMap map(state.range(0));
    for (int i = 0; i < state.range(0); ++i) {
        map.insert({gen() & ~mask, gen()});
    }

    std::vector<int> queries(state.range(1));
    for (auto &x: queries)
        x = gen() | mask;

    assert(queries.size() == state.range(1));

    for (auto _: state) {
        for (auto x: queries) {
            benchmark::DoNotOptimize(map.insert({x, x}));
        }
        state.PauseTiming();
        for (auto &x: queries) {
            map.erase(x);
            x = gen() | mask;
        }
        state.ResumeTiming();
    }

    state.SetComplexityN(state.range(1));
}

BENCHMARK(BM_BatchInsert<epcpp::hash_map>)->ArgsProduct(
        {benchmark::CreateRange(256, 1 << 20, 64), benchmark::CreateRange(10, 1000, 10)})->Complexity();
BENCHMARK(BM_BatchInsert<epcpp::std_hash_map>)->ArgsProduct(
        {benchmark::CreateRange(256, 1 << 20, 64), benchmark::CreateRange(10, 1000, 10)})->Complexity();


BENCHMARK_MAIN();
