
#include <gtest/gtest.h>

#include "atomic_shared_ptr.h"

struct Counter {
    Counter(std::atomic<long> &num_constructor, std::atomic<long> &num_destructor)
            : m_num_constructor(num_constructor), m_num_destructor(num_destructor) {
        m_num_constructor.fetch_add(1);
    }

    Counter(const Counter &other)
            : m_num_constructor(other.m_num_constructor), m_num_destructor(other.m_num_destructor) {
        m_num_constructor.fetch_add(1);
    }

    ~Counter() {
        m_num_destructor.fetch_add(1);
    }

    std::atomic<long> &m_num_constructor;
    std::atomic<long> &m_num_destructor;
};


TEST(SharedPointerTest, v2Constructors) {
    using epcpp::atomic::shared_ptr;

    std::atomic<long> num_constructor = 0;
    std::atomic<long> num_destructor = 0;

    ASSERT_EQ(num_constructor.load(), 0);
    ASSERT_EQ(num_destructor.load(), 0);

    auto ptr1 = shared_ptr<Counter>::make_shared(num_constructor, num_destructor);

    ASSERT_EQ(num_constructor.load(), 1);
    ASSERT_EQ(num_destructor.load(), 0);
    ASSERT_EQ(ptr1.use_count(), 1);

    auto ptr2(ptr1);
    ASSERT_EQ(ptr1.use_count(), 2);

    shared_ptr<Counter> ptr3;
    ptr3 = std::move(ptr2);

    ASSERT_EQ(ptr2.get(), nullptr);
    ASSERT_EQ(ptr1.use_count(), 2);

    auto ptr4 = ptr3;

    ASSERT_EQ(ptr1.use_count(), 3);

    shared_ptr<Counter> ptr5;
    ptr5 = std::move(ptr4);

    ASSERT_EQ(ptr4.get(), nullptr);
    ASSERT_EQ(ptr1.use_count(), 3);

    ptr5.reset();

    ASSERT_EQ(ptr5.get(), nullptr);
    ASSERT_EQ(num_constructor.load(), 1);
    ASSERT_EQ(num_destructor.load(), 0);
    ASSERT_EQ(ptr1.use_count(), 2);

    ptr3.reset();

    ASSERT_EQ(ptr3.get(), nullptr);
    ASSERT_EQ(num_destructor.load(), 0);
    ASSERT_EQ(ptr1.use_count(), 1);

    ptr1.reset();

    ASSERT_EQ(ptr1.get(), nullptr);
    ASSERT_EQ(num_constructor.load(), 1);
    ASSERT_EQ(num_destructor.load(), 1);
}



TEST(SharedPointerTest, LinkedList) {
    using epcpp::atomic::shared_ptr;

    struct Node {
        explicit Node(const Counter &counter) : counter(counter) {}

        Counter counter;
        shared_ptr<Node> next;
    };

    std::atomic<long> num_constructor = 0;
    std::atomic<long> num_destructor = 0;

    Counter counter(num_constructor, num_destructor);
    ASSERT_EQ(num_constructor.load(), 1);
    ASSERT_EQ(num_destructor.load(), 0);

    shared_ptr<Node> head = shared_ptr<Node>::make_shared(counter);

    ASSERT_EQ(num_constructor.load(), 2);
    ASSERT_EQ(num_destructor.load(), 0);

    auto it = head;
    for (int i = 0; i < 10; ++i) {
        it->next = shared_ptr<Node>::make_shared(counter);
        it = it->next;

        ASSERT_EQ(num_constructor.load(), i + 3);
        ASSERT_EQ(num_destructor.load(), 0);
    }

    ASSERT_EQ(num_constructor.load(), 12);
    ASSERT_EQ(num_destructor.load(), 0);

    auto mid = it;
    for (int i = 10; i < 20; ++i) {
        it->next = shared_ptr<Node>::make_shared(counter);
        it = it->next;

        ASSERT_EQ(num_constructor.load(), i + 3);
        ASSERT_EQ(num_destructor.load(), 0);
    }

    ASSERT_EQ(num_constructor.load(), 22);
    ASSERT_EQ(num_destructor.load(), 0);

    it.reset();
    head.reset();

    ASSERT_EQ(num_constructor.load(), 22);
    ASSERT_EQ(num_destructor.load(), 10);

    mid.reset();

    ASSERT_EQ(num_constructor.load(), 22);
    ASSERT_EQ(num_destructor.load(), 21);
}


TEST(AtomicSharedPointerTest, AtomicCompareExchange) {
    using epcpp::atomic::shared_ptr;
    using epcpp::atomic::atomic;

    std::atomic<long> num_constructor = 0;
    std::atomic<long> num_destructor = 0;

    ASSERT_EQ(num_constructor.load(), 0);
    ASSERT_EQ(num_destructor.load(), 0);

    auto ptr_a = shared_ptr<Counter>::make_shared(num_constructor, num_destructor);

    ASSERT_EQ(num_constructor.load(), 1);
    ASSERT_EQ(num_destructor.load(), 0);
    ASSERT_EQ(ptr_a.use_count(), 1);

    atomic<shared_ptr<Counter>> atomic_ptr(ptr_a);

    ASSERT_EQ(ptr_a.use_count(), 2);

    auto ptr2 = atomic_ptr.load();

    ASSERT_EQ(ptr2.get(), ptr_a.get());
    ASSERT_EQ(ptr_a.use_count(), 3);


    auto ptr_b= shared_ptr<Counter>::make_shared(num_constructor, num_destructor);
    auto ptr_c= shared_ptr<Counter>::make_shared(num_constructor, num_destructor);
    ASSERT_EQ(num_constructor.load(), 3);
    ASSERT_EQ(num_destructor.load(), 0);

    ASSERT_EQ(ptr_a.use_count(), 3);
    ASSERT_EQ(ptr_b.use_count(), 1);
    ASSERT_EQ(ptr_c.use_count(), 1);

    {
        // a (nullptr => nullptr)
        shared_ptr<Counter> expected;

        ASSERT_EQ(ptr_a.use_count(), 3);
        ASSERT_EQ(ptr_b.use_count(), 1);
        ASSERT_EQ(ptr_c.use_count(), 1);

        bool exchange_success = atomic_ptr.compare_exchange_strong(expected, shared_ptr<Counter>());
        ASSERT_FALSE(exchange_success);

        ASSERT_EQ(expected.get(), atomic_ptr.load().get());

        ASSERT_EQ(ptr_a.use_count(), 4);
        ASSERT_EQ(ptr_b.use_count(), 1);
        ASSERT_EQ(ptr_c.use_count(), 1);
    }

    {
        // a (b => nullptr)
        shared_ptr<Counter> expected = ptr_b;

        ASSERT_EQ(ptr_a.use_count(), 3);
        ASSERT_EQ(ptr_b.use_count(), 2);
        ASSERT_EQ(ptr_c.use_count(), 1);

        bool exchange_success = atomic_ptr.compare_exchange_strong(expected, shared_ptr<Counter>());
        ASSERT_FALSE(exchange_success);

        ASSERT_EQ(expected.get(), atomic_ptr.load().get());

        ASSERT_EQ(ptr_a.use_count(), 4);
        ASSERT_EQ(ptr_b.use_count(), 1);
        ASSERT_EQ(ptr_c.use_count(), 1);
    }

    {
        // a (nullptr => b)
        shared_ptr<Counter> expected;

        ASSERT_EQ(ptr_a.use_count(), 3);
        ASSERT_EQ(ptr_b.use_count(), 1);
        ASSERT_EQ(ptr_c.use_count(), 1);

        bool exchange_success = atomic_ptr.compare_exchange_strong(expected, ptr_b);
        ASSERT_FALSE(exchange_success);

        ASSERT_EQ(expected.get(), atomic_ptr.load().get());

        ASSERT_EQ(ptr_a.use_count(), 4);
        ASSERT_EQ(ptr_b.use_count(), 1);
        ASSERT_EQ(ptr_c.use_count(), 1);
    }

    {
        // a (b => c)
        shared_ptr<Counter> expected = ptr_b;

        ASSERT_EQ(ptr_a.use_count(), 3);
        ASSERT_EQ(ptr_b.use_count(), 2);
        ASSERT_EQ(ptr_c.use_count(), 1);

        bool exchange_success = atomic_ptr.compare_exchange_strong(expected, ptr_c);
        ASSERT_FALSE(exchange_success);

        ASSERT_EQ(expected.get(), atomic_ptr.load().get());

        ASSERT_EQ(ptr_a.use_count(), 4);
        ASSERT_EQ(ptr_b.use_count(), 1);
        ASSERT_EQ(ptr_c.use_count(), 1);
    }

    {
        // a (a => b)
        shared_ptr<Counter> expected = ptr_a;

        ASSERT_EQ(ptr_a.use_count(), 4);
        ASSERT_EQ(ptr_b.use_count(), 1);
        ASSERT_EQ(ptr_c.use_count(), 1);

        bool exchange_success = atomic_ptr.compare_exchange_strong(expected, ptr_b);
        ASSERT_TRUE(exchange_success);

        ASSERT_EQ(expected.get(), ptr_a.get());
        ASSERT_EQ(atomic_ptr.load().get(), ptr_b.get());

        ASSERT_EQ(ptr_a.use_count(), 3);
        ASSERT_EQ(ptr_b.use_count(), 2);
        ASSERT_EQ(ptr_c.use_count(), 1);
    }

    {
        // b (b => c)
        shared_ptr<Counter> expected = ptr_b;

        ASSERT_EQ(ptr_a.use_count(), 2);
        ASSERT_EQ(ptr_b.use_count(), 3);
        ASSERT_EQ(ptr_c.use_count(), 1);

        bool exchange_success = atomic_ptr.compare_exchange_strong(expected, ptr_c);
        ASSERT_TRUE(exchange_success);

        ASSERT_EQ(expected.get(), ptr_b.get());
        ASSERT_EQ(atomic_ptr.load().get(), ptr_c.get());

        ASSERT_EQ(ptr_a.use_count(), 2);
        ASSERT_EQ(ptr_b.use_count(), 2);
        ASSERT_EQ(ptr_c.use_count(), 2);
    }

    ASSERT_EQ(atomic_ptr.load().get(), ptr_c.get());
    ASSERT_EQ(ptr2.get(), ptr_a.get());

    ASSERT_EQ(ptr_a.use_count(), 2);
    ASSERT_EQ(ptr_b.use_count(), 1);
    ASSERT_EQ(ptr_c.use_count(), 2);

    ptr2.reset();
    atomic_ptr.store(shared_ptr<Counter>());

    ASSERT_EQ(ptr_a.use_count(), 1);
    ASSERT_EQ(ptr_b.use_count(), 1);
    ASSERT_EQ(ptr_c.use_count(), 1);

    ASSERT_EQ(num_constructor.load(), 3);
    ASSERT_EQ(num_destructor.load(), 0);

    ptr_a.reset();
    ptr_b.reset();
    ptr_c.reset();

    ASSERT_EQ(num_constructor.load(), 3);
    ASSERT_EQ(num_destructor.load(), 3);
}
