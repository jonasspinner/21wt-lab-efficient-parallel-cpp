
#include <gtest/gtest.h>

#include <barrier>

#include "hash_map.h"
#include "bloom_filter.h"


template<class HashMap>
class HashMapTest : public ::testing::Test {
};

using HashMapTypes = ::testing::Types<
        epcpp::hash_map<int, int>,
        epcpp::HashMap<int, int, std::hash<int>, epcpp::BloomFilterAdapter<epcpp::ListBucket<int, int, epcpp::atomic_marked_list, std::equal_to<>, std::allocator<std::pair<const int, int>>, false>>>,
        epcpp::HashMap<int, int, std::hash<int>, epcpp::BloomFilterAdapter<epcpp::ListBucket<int, int, epcpp::atomic_marked_list, std::equal_to<>, std::allocator<std::pair<const int, int>>, false>>>,
        epcpp::HashMap<int, int, std::hash<int>, epcpp::ListBucket<int, int, epcpp::single_mutex_list, std::equal_to<>, std::allocator<std::pair<const int, int>>, false>>,
        epcpp::hash_map_a<int, int>,
        epcpp::hash_map_b<int, int>,
        epcpp::hash_map_c<int, int>,
        epcpp::hash_map_std<int, int>
>;
TYPED_TEST_SUITE(HashMapTest, HashMapTypes);

TYPED_TEST(HashMapTest, Find) {
    TypeParam map(10);

    int k = 100;

    for (int i = 0; i < k; ++i) {
        auto[h, inserted] = map.insert({i, 2 * i});
        //ASSERT_TRUE(h);
        ASSERT_NE(h, map.end());
        ASSERT_TRUE(inserted);
        ASSERT_EQ(h->first, i);
        ASSERT_EQ(h->second, 2 * i);

        //h.reset();
        //ASSERT_FALSE(h);
        //ASSERT_EQ(h, map.end());
    }

    for (int i = 0; i < k; ++i) {
        auto h = map.find(i);
        //ASSERT_TRUE(h);
        ASSERT_NE(h, map.end());
        ASSERT_EQ(h->first, i);
        ASSERT_EQ(h->second, 2 * i);

        //h.reset();
        //ASSERT_FALSE(h);
        //ASSERT_EQ(h, map.end());
    }

    for (int i = k; i < 2 * k; ++i) {
        auto h = map.find(i);
        //ASSERT_FALSE(h);
        ASSERT_EQ(h, map.end());
    }

    for (int i = 0; i < k; ++i) {
        auto[h, inserted] = map.insert({i, 3 * i});
        h->second = 3 * i;
        //ASSERT_TRUE(h);
        ASSERT_NE(h, map.end());
        ASSERT_FALSE(inserted);
        ASSERT_EQ(h->first, i);
        ASSERT_EQ(h->second, 3 * i);

        //h.reset();
        //ASSERT_FALSE(h);
        //ASSERT_EQ(h, map.end());
    }

    for (int i = 0; i < k; ++i) {
        auto h = map.find(i);
        //ASSERT_TRUE(h);
        ASSERT_NE(h, map.end());
        ASSERT_EQ(h->first, i);
        ASSERT_EQ(h->second, 3 * i);

        //h.reset();
        //ASSERT_FALSE(h);
        //ASSERT_EQ(h, map.end());
    }

    for (int i = k; i < 2 * k; ++i) {
        auto h = map.find(i);
        //ASSERT_FALSE(h);
        ASSERT_EQ(h, map.end());
    }

    for (int i = k; i < 2 * k; ++i) {
        auto erased = map.erase(i);
        ASSERT_FALSE(erased);
    }

    for (int i = 0; i < k; ++i) {
        auto erased = map.erase(i);
        ASSERT_TRUE(erased) << i;
    }
}


TYPED_TEST(HashMapTest, ConcurrentInsertErase) {
    std::size_t num_threads = 10;
    std::size_t num_elements_per_thread = 100;
    std::size_t num_find_repeats = 10;

    TypeParam map(num_threads * num_elements_per_thread);


    std::barrier barrier(num_threads + 1);

    auto work = [&](std::size_t thread_idx) {
        barrier.arrive_and_wait();

        auto start = thread_idx * num_elements_per_thread;
        auto end = (thread_idx + 1) * num_elements_per_thread;

        for (std::size_t iteration = 0; iteration < num_find_repeats; ++iteration) {

            for (std::size_t j = start; j < end; ++j) {
                auto[handle, inserted] = map.insert({j, thread_idx});

                ASSERT_TRUE(inserted) << "thread_idx " << thread_idx << " j=" << j;
                ASSERT_NE(handle, map.end());
                ASSERT_EQ(handle->first, j);
                ASSERT_EQ(handle->second, thread_idx);
            }

            for (std::size_t j = start; j < end; ++j) {
                auto h = map.find(j);

                ASSERT_NE(h, map.end());
            }

            for (std::size_t j = start; j < end; ++j) {
                auto erased = map.erase(j);

                ASSERT_TRUE(erased);
            }

            for (std::size_t j = start; j < end; ++j) {
                auto h = map.find(j);

                ASSERT_EQ(h, map.end()) << j << " " << h->first << " " << h->second;
            }
        }
    };

    std::vector<std::thread> threads;
    for (std::size_t thread_idx = 0; thread_idx < num_threads; ++thread_idx) {
        threads.emplace_back(work, thread_idx);
    }

    barrier.arrive_and_wait();

    for (auto &thread: threads) {
        thread.join();
    }
}


template<class List>
class CustomAllocatorHashMapTest : public ::testing::Test {
};

using CustomAllocatorHashMapTypes = ::testing::Types<
        epcpp::hash_map_a<std::string, std::size_t, std::hash<std::string>, std::equal_to<>, epcpp::utils::debug_allocator::CountingAllocator<std::pair<const std::string, std::size_t>>>,
        epcpp::hash_map_b<std::string, std::size_t, std::hash<std::string>, std::equal_to<>, epcpp::utils::debug_allocator::CountingAllocator<std::pair<const std::string, std::size_t>>>,
        epcpp::hash_map_c<std::string, std::size_t, std::hash<std::string>, std::equal_to<>, epcpp::utils::debug_allocator::CountingAllocator<std::pair<const std::string, std::size_t>>>
>;
TYPED_TEST_SUITE(CustomAllocatorHashMapTest, CustomAllocatorHashMapTypes);


TYPED_TEST(CustomAllocatorHashMapTest, CustomAllocator) {
    using Allocator = typename TypeParam::allocator_type;


    std::pair<std::string, std::size_t> element;
    Allocator alloc;

    {
        TypeParam map(10, alloc);

        ASSERT_EQ(alloc.counts().num_allocate, 0);
        ASSERT_EQ(alloc.counts().num_deallocate, 0);

        auto[h1, inserted] = map.insert(element);
        ASSERT_NE(h1, map.end());
        ASSERT_EQ(h1->first, element.first);
        ASSERT_EQ(h1->second, element.second);

        ASSERT_EQ(alloc.counts().num_allocate, 1);

        auto h2 = map.find(element.first);
        ASSERT_NE(h2, map.end());
        ASSERT_EQ(h1->first, element.first);
        ASSERT_EQ(h1->second, element.second);

        auto erased = map.erase(element.first);
        ASSERT_TRUE(erased);
    }
    ASSERT_EQ(alloc.counts().num_allocate, 1);
    ASSERT_EQ(alloc.counts().num_deallocate, 1);
}