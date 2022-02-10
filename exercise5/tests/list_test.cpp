#include <gtest/gtest.h>
#include <thread>
#include <barrier>

#include "tbb/scalable_allocator.h"

#include "lists/single_mutex_list.h"
#include "lists/_leaking_atomic_list.h"
#include "lists/node_mutex_list.h"
#include "lists/_atomic_shared_ptr_list.h"
#include "lists/atomic_marked_list.h"
#include "utils.h"

struct ValueType {
};
template<template<class> class A>
concept TestConcept = requires(A<ValueType> a) {
    { a.hello() } -> std::same_as<int>;
    { a.value() } -> std::same_as<ValueType>;
    std::same_as<typename A<ValueType>::pointer, ValueType *>;
};

template<class V>
struct B {
    using pointer = V *;

    int hello() { return 0; }

    V value() { return m_value; }

    V m_value;
};

static_assert(TestConcept<B>);


template<class List>
class ListTest : public ::testing::Test {
};

using ListTypes = ::testing::Types<
        epcpp::single_mutex_list<int>,
        epcpp::node_mutex_list<int>,
        epcpp::atomic_marked_list<int>,
        epcpp::single_mutex_list<int, tbb::scalable_allocator<int>>,
        epcpp::node_mutex_list<int, tbb::scalable_allocator<int>>,
        epcpp::atomic_marked_list<int, tbb::scalable_allocator<int>>
>;
TYPED_TEST_SUITE(ListTest, ListTypes);

TYPED_TEST(ListTest, InsertFind) {
    TypeParam list;

    for (int i = 0; i < 10; ++i) {
        auto[handle, inserted] = list.insert(i);

        ASSERT_TRUE(inserted);
        ASSERT_NE(handle, list.end());
        ASSERT_EQ(*handle, i);
    }

    for (int i = 0; i < 10; ++i) {
        auto handle = list.find(i);

        ASSERT_NE(handle, list.end());
        ASSERT_EQ(*handle, i);
    }

    for (int i = 10; i < 20; ++i) {
        auto handle = list.find(i);

        ASSERT_EQ(handle, list.end());
    }

    for (int i = 0; i < 10; ++i) {
        auto[handle, inserted] = list.insert(i);

        ASSERT_FALSE(inserted) << i;
        ASSERT_NE(handle, list.end());
        ASSERT_EQ(*handle, i);
    }

}


TYPED_TEST(ListTest, Erase) {
    int num_elements = 100;

    TypeParam list;

    for (int i = 0; i < num_elements; ++i) {
        auto[handle, inserted] = list.insert(i);

        ASSERT_TRUE(inserted);
        ASSERT_NE(handle, list.end());
        ASSERT_EQ(*handle, i);
    }

    for (int i = 0; i < num_elements; ++i) {
        auto erased = list.erase(i);

        ASSERT_TRUE(erased);
    }

    for (int i = 0; i < num_elements; ++i) {
        auto[handle, inserted] = list.insert(i);

        ASSERT_TRUE(inserted) << i;
        ASSERT_NE(handle, list.end());
        ASSERT_EQ(*handle, i);
    }
}


TYPED_TEST(ListTest, ConcurrentInsertFind) {

    std::size_t num_threads = 10;
    std::size_t num_elements_per_thread = 100;
    std::size_t num_find_repeats = 10;

    std::barrier barrier(num_threads);

    TypeParam list;

    auto work = [&](std::size_t thread_idx) {
        barrier.arrive_and_wait();

        auto start = thread_idx * num_elements_per_thread;
        auto end = (thread_idx + 1) * num_elements_per_thread;
        for (std::size_t j = start; j < end; ++j) {
            auto[handle, inserted] = list.insert(j);

            ASSERT_TRUE(inserted);
            ASSERT_NE(handle, list.end());
            ASSERT_EQ(*handle, j);
        }

        for (std::size_t iteration = 0; iteration < num_find_repeats; ++iteration) {
            for (std::size_t j = start; j < end; ++j) {
                auto handle = list.find(j);

                ASSERT_NE(handle, list.end());
                ASSERT_EQ(*handle, j);
            }
        }
    };

    std::vector<std::thread> threads;
    for (std::size_t thread_idx = 0; thread_idx < num_threads; ++thread_idx) {
        threads.template emplace_back(work, thread_idx);
    }

    for (auto &thread: threads) {
        thread.join();
    }

}


TYPED_TEST(ListTest, ConcurrentInsertErase) {

    std::size_t num_threads = 10;
    std::size_t num_elements_per_thread = 100;
    std::size_t num_find_repeats = 10;

    std::barrier barrier(num_threads + 1);

    TypeParam list;

    auto work = [&](std::size_t thread_idx) {
        barrier.arrive_and_wait();

        auto start = thread_idx * num_elements_per_thread;
        auto end = (thread_idx + 1) * num_elements_per_thread;

        for (std::size_t iteration = 0; iteration < num_find_repeats; ++iteration) {

            for (std::size_t j = start; j < end; ++j) {
                auto[handle, inserted] = list.insert((int)j);

                ASSERT_TRUE(inserted) << "thread_idx " << thread_idx << " j=" << j;
                ASSERT_NE(handle, list.end());
                ASSERT_EQ(*handle, j);
            }

            for (std::size_t j = start; j < end; ++j) {
                auto h = list.find((int)j);

                ASSERT_NE(h, list.end());
            }

            for (std::size_t j = start; j < end; ++j) {
                auto erased = list.erase((int)j);

                ASSERT_TRUE(erased);
            }

            for (std::size_t j = start; j < end; ++j) {
                auto h = list.find((int)j);

                ASSERT_EQ(h, list.end()) << j << " " << *h;
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


TYPED_TEST(ListTest, ConcurrentSingleValueInsertErase) {

    std::size_t num_threads = 10;
    std::size_t num_repeats = 10000;

    std::barrier barrier(num_threads + 1);

    TypeParam list;

    int element = 0;

    auto work = [&]() {
        barrier.arrive_and_wait();
        for (std::size_t iteration = 0; iteration < num_repeats; ++iteration) {
            auto[handle, inserted] = list.insert(element);
            ASSERT_NE(handle, list.end());
            ASSERT_EQ(*handle, element);

            [[maybe_unused]] auto erased = list.erase(element);
        }
    };

    std::vector<std::thread> threads;
    for (std::size_t thread_idx = 0; thread_idx < num_threads; ++thread_idx) {
        threads.emplace_back(work);
    }

    barrier.arrive_and_wait();


    for (auto &thread: threads) {
        thread.join();
    }
}


TEST(ListTest, AtomicValueType) {
    epcpp::atomic_marked_list<std::atomic<int>> list;

    {
        auto[h, inserted] = list.insert(2);

        ASSERT_TRUE(inserted);
        ASSERT_TRUE(h);
        h->fetch_add(1);
        *h += 1;
    }
    {
        auto h = list.find(2);
        ASSERT_EQ(h, list.end());
    }
}



template<class List>
class CustomAllocatorListTest : public ::testing::Test {
};

using CustomAllocatorListTypes = ::testing::Types<
        epcpp::single_mutex_list<std::array<std::size_t, 64>, epcpp::utils::debug_allocator::CountingAllocator<std::array<std::size_t, 64>>>,
        epcpp::node_mutex_list<std::array<std::size_t, 64>, epcpp::utils::debug_allocator::CountingAllocator<std::array<std::size_t, 64>>>,
        epcpp::atomic_marked_list<std::array<std::size_t, 64>, epcpp::utils::debug_allocator::CountingAllocator<std::array<std::size_t, 64>>>
>;
TYPED_TEST_SUITE(CustomAllocatorListTest, CustomAllocatorListTypes);

TYPED_TEST(CustomAllocatorListTest, CustomAllocator) {
    using Allocator = epcpp::utils::debug_allocator::CountingAllocator<std::array<std::size_t, 64>>;

    std::array<std::size_t, 64> element{};

    Allocator alloc;
    {
        TypeParam list(alloc);

        ASSERT_EQ(alloc.counts().num_allocate, 0);
        ASSERT_EQ(alloc.counts().num_deallocate, 0);

        auto[h1, inserted] = list.insert(element);
        ASSERT_NE(h1, list.end());
        ASSERT_EQ(*h1, element);

        ASSERT_EQ(alloc.counts().num_allocate, 1);

        auto h2 = list.find(element);
        ASSERT_NE(h2, list.end());
        ASSERT_EQ(*h2, element);

        auto erased = list.erase(element);
        ASSERT_TRUE(erased);
    }
    ASSERT_EQ(alloc.counts().num_allocate, 1);
    ASSERT_EQ(alloc.counts().num_deallocate, 1);
}

TYPED_TEST(CustomAllocatorListTest, InVector) {
    using Allocator = epcpp::utils::debug_allocator::CountingAllocator<std::array<std::size_t, 64>>;

    std::array<std::size_t, 64> element{};

    Allocator alloc;

    std::size_t num_lists = 16;
    {
        TypeParam * vec = std::allocator<TypeParam>{}.allocate(num_lists);

        for (std::size_t i = 0; i < num_lists; ++i) {
            std::construct_at(&vec[i], alloc);
        }

        for (std::size_t i = 0; i < num_lists; ++i) {
            ASSERT_TRUE(vec[i].empty());
            ASSERT_EQ(vec[i].find(element), vec[i].end());
        }
    }
}