#pragma once

#include <cstddef>
#include <memory>
#include <atomic>
#include <thread>
#include <cassert>
#include <iostream>


#warning "ConcurrentQueueCASElement has bugs and should not be used"


template <class T>
class ConcurrentQueueCASElement {
    /**
     * This is an attempt to build a queue that utilizes CAS(0 -> T) for push and CAS(T -> 0) for pop.
     *
     * Unfortunately there is a bug and elements get lost in the queue.
     */
public:
    explicit ConcurrentQueueCASElement(std::size_t capacity) : m_capacity(capacity) {
        m_elements = std::make_unique<std::atomic<T>[]>(m_capacity);
    }

    // fully concurrent
    void push(const T& e) {
        assert(e != T{});
        while (!try_push(e)) {
            std::this_thread::yield();
        }
    }

    // fully concurrent
    T    pop() {
        T e;
        while (!try_pop(e)) {
            std::this_thread::yield();
        }
        assert(e != T{});
        return e;
    }

    // Whatever other functions you want
    bool try_push(const T& e) {
        auto old_head = m_head.load();
        auto old_tail = m_tail.load();
        auto idx = old_head;
        T expected = T{};
        while (true) {
            if (idx >= old_tail + m_capacity) {
                assert(m_tail <= m_head);
                return false;
            }

            if (m_elements[idx % m_capacity].compare_exchange_strong(expected, e)) {
                assert(m_tail <= m_head);
                m_head.fetch_add(1);
                assert(m_tail <= m_head);
                return true;
            }
            idx++;
        }
    }

    bool try_pop(T &e) {
        auto old_tail = m_tail.load();
        auto old_head = m_head.load();
        auto idx = old_tail;
        while (true) {
            if (idx >= old_head) {
                assert(m_tail <= m_head);
                return false;
            }

            e = m_elements[idx % m_capacity].load();
            if (e != T{} && m_elements[idx % m_capacity].compare_exchange_strong(e, T{})) {
                assert(e != T{});
                assert(m_tail <= m_head);
                m_tail.fetch_add(1);
                assert(m_tail <= m_head);
                return true;
            }
            idx++;
        }
    }

    [[nodiscard]] bool empty() const {
        return m_tail.load() == m_head.load();
    }

    void reset() {
        m_head = 0;
        m_tail = 0;

        for (size_t idx = 0; idx < m_capacity; ++idx) {
            m_elements[idx] = T{};
        }
    }

    void print_state() {
        std::cout << m_capacity << " " << m_tail << " " << m_tail << std::endl;
        for (size_t idx = 0; idx < m_capacity; ++idx) {
            std::cout << m_elements[idx] << " ";
        }
        std::cout << std::endl;
    }

private:
    std::unique_ptr<std::atomic<T>[]> m_elements;

    std::size_t m_capacity;

    std::atomic<unsigned long> m_head{0};
    std::atomic<unsigned long> m_tail{0};
};
