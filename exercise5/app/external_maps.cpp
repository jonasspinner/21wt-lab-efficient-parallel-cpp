#include <iostream>

#include "tbb/concurrent_hash_map.h"
#include "tbb/concurrent_unordered_map.h"
#include "tbb/scalable_allocator.h"


int main() {
    tbb::scalable_allocator<int> alloc;

    oneapi::tbb::concurrent_hash_map<int, int> map1;

    map1.insert({0, 1});

    std::cout << map1.size() << std::endl;

    oneapi::tbb::concurrent_unordered_map<int, int> map2;

    auto [h, inserted] = map2.insert({0, 1});

    std::cout << h->first << " " << h->second << std::endl;

    return 0;
}