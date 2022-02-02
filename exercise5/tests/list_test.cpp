#include <gtest/gtest.h>
#include <thread>
#include <barrier>

#include "lists/single_mutex_list.h"
#include "lists/leaking_atomic_list.h"
#include "lists/node_mutex_list.h"
#include "lists/atomic_shared_ptr_list.h"

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
        epcpp::leaking_atomic_list<int>,
        epcpp::node_mutex_list<int>,
        epcpp::atomic_shared_ptr_list<int>
>;
TYPED_TEST_SUITE(ListTest, ListTypes);

TYPED_TEST(ListTest, InsertFind) {
    typename TypeParam::node_manager node_manager;
    TypeParam list(node_manager);

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


TYPED_TEST(ListTest, ConcurrentInsertFind) {

    std::size_t num_threads = 10;
    std::size_t num_elements_per_thread = 100;
    std::size_t num_find_repeats = 10;

    std::barrier barrier(num_threads);

    typename TypeParam::node_manager node_manager;
    TypeParam list(node_manager);

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

    for (auto &thread : threads) {
        thread.join();
    }

}



