#pragma once

#include <cstddef>
#include <memory>
#include <atomic>
#include <thread>
#include <cassert>
#include <mutex>
#include <iostream>


template<class T, bool enable_debug_mutex = false>
class ConcurrentQueue {
    /**
     * This queue is separated into producer and a consumer part with head and tail each.
     * Elements are inserted at producer_head and elements are inserted at consumer_tail.
     *
     * Heads and tails are only ever incremented. Index into the elements is done with _ % capacity.
     *
     * Invariant:
     *      producer_tail <= consumer_tail <= consumer_head <= producer_head
     *
     * This should be adaptable for signed overflow, but the current implementation assumes that no overflow occurs.
     */
public:
    explicit ConcurrentQueue(std::size_t capacity) : m_capacity(capacity) {
        m_elements = std::make_unique<std::atomic<T>[]>(m_capacity);
    }

    // fully concurrent
    void push(const T &e) {
        assert(e != T{});
        while (!try_push(e)) {
            std::this_thread::yield();
        }
    }

    // fully concurrent
    T pop() {
        T e;
        while (!try_pop(e)) {
            std::this_thread::yield();
        }
        assert(e != T{});
        return e;
    }

    // Whatever other functions you want
    bool try_push(const T &e) {
        std::scoped_lock lock(m_debug_mutex);

        auto old_producer_head = m_producer_head.load();
        unsigned long new_producer_head;

        do {
            new_producer_head = old_producer_head + 1;
            if (new_producer_head > m_producer_tail.load() + m_capacity) {
                check_invariants();
                return false;
            }
        } while (!m_producer_head.compare_exchange_weak(old_producer_head, new_producer_head));

        assert(old_producer_head + 1 == new_producer_head);

        m_elements[old_producer_head % m_capacity] = e;


        // Now wait for turn to update m_consumer_head
        size_t num_repeats = 0;
        while (true) {
            num_repeats++;

            auto expected = old_producer_head;
            if (m_consumer_head.compare_exchange_weak(expected, new_producer_head)) {

                assert(m_consumer_head >= new_producer_head);
                check_invariants();

                return true;
            }

            if (num_repeats == 16) {
                num_repeats = 0;
                std::this_thread::yield();
            }
        }
    }

    bool try_pop(T &e) {
        std::scoped_lock lock(m_debug_mutex);

        auto old_consumer_tail = m_consumer_tail.load();
        unsigned long new_consumer_tail;

        do {
            new_consumer_tail = old_consumer_tail + 1;
            if (new_consumer_tail > m_consumer_head.load()) {
                check_invariants();
                return false;
            }
        } while (!m_consumer_tail.compare_exchange_weak(old_consumer_tail, new_consumer_tail));
        assert(old_consumer_tail + 1 == new_consumer_tail);

        e = m_elements[new_consumer_tail % m_capacity];


        // Now wait for turn to update producer_tail
        size_t num_repeats = 0;
        while (true) {
            num_repeats++;

            auto expected = old_consumer_tail;
            if (m_producer_tail.compare_exchange_weak(expected, new_consumer_tail)) {

                assert(m_producer_tail >= new_consumer_tail);
                check_invariants();

                return true;
            }

            if (num_repeats == 16) {
                num_repeats = 0;
                std::this_thread::yield();
            }
        }
    }

    [[nodiscard]] bool empty() const {
        // NOTE: not thread-safe
        return size() == 0;
    }

    [[nodiscard]] size_t size() const {
        // NOTE: not thread-safe
        return m_consumer_head.load() - m_consumer_tail.load();
    }

    void reset() {
        m_producer_head = 0;
        m_producer_tail = 0;

        m_consumer_head = 0;
        m_consumer_tail = 0;

        for (size_t idx = 0; idx < m_capacity; ++idx) {
            m_elements[idx] = T{};
        }
    }

    void print_state() {
        std::cout << m_capacity << std::endl;
        std::cout << m_producer_tail << " " << m_producer_head << std::endl;
        std::cout << m_consumer_tail << " " << m_consumer_head << std::endl;
        for (size_t idx = 0; idx < m_capacity; ++idx) {
            std::cout << m_elements[idx] << " ";
        }
        std::cout << std::endl;
    }

private:
    void check_invariants() {
        // NOTE: m_debug_mutex must be locked
        // NOTE: Assumes no overflow occured.
        if constexpr (enable_debug_mutex) {
            assert(m_producer_tail <= m_consumer_tail);
            assert(m_consumer_tail <= m_consumer_head);
            assert(m_consumer_head <= m_producer_head);
        }
    }

    std::unique_ptr<std::atomic<T>[]> m_elements;

    struct MockMutex {
        void lock() {}
        void unlock() {}
    };
    std::conditional_t<enable_debug_mutex, std::mutex, MockMutex> m_debug_mutex{};

    std::size_t m_capacity;

    alignas(64) std::atomic<unsigned long> m_producer_head{0};
    std::atomic<unsigned long> m_producer_tail{0};

    alignas(64) std::atomic<unsigned long> m_consumer_head{0};
    std::atomic<unsigned long> m_consumer_tail{0};
};
