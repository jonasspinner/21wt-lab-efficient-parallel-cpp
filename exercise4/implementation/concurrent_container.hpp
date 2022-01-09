#pragma once

#include <cstddef>
#include <memory>
#include <random>
#include <atomic>

template<class T, bool keep_stats = false>
class ConcurrentContainer {
    /**
     * Keeps an array of elements. Every thread keeps an thread-local offset managed by thread_offset().
     *
     * Producers walk until they can CAS(0 -> T) and consumers until CAS(T -> 0).
     * If the array is empty or full, the producers/consumers walk every element at most once, before reporting an error.
     */
public:
    explicit ConcurrentContainer(std::size_t capacity) : m_capacity(capacity) {
        m_elements = std::make_unique<std::atomic<T>[]>(m_capacity);
    }

    // fully concurrent
    void push(const T &e) {
        size_t search_start = thread_offset(false, 0);
        size_t search_size = m_capacity;
        while (!try_push(e, search_start, search_size)) {
            std::this_thread::yield();
        }
        thread_offset(true, search_start);
    }

    // fully concurrent
    T pop() {
        T e;
        size_t search_start = thread_offset(false, 0);
        size_t search_size = m_capacity;
        while (!try_pop(e, search_start, search_size)) {
            std::this_thread::yield();
        }
        thread_offset(true, search_start);
        return e;
    }

    // Whatever other functions you want

    bool try_push(const T &e, size_t &search_start, size_t search_size) {
        for (size_t i = search_start; i < search_start + search_size; ++i) {
            size_t idx = i % m_capacity;
            T expected = T{};
            if (m_elements[idx].compare_exchange_strong(expected, e)) {
                //std::cout << "push " << (i - search_start) << std::endl;
                search_start = i;
                if constexpr(keep_stats) { m_size++; }
                return true;
            }
        }
        //std::cout << "push failed" << std::endl;
        return false;
    }

    bool try_pop(T &e, size_t &search_start, size_t search_size) {
        for (size_t i = search_start; i < search_start + search_size; ++i) {
            size_t idx = i % m_capacity;
            e = m_elements[idx].load();
            if (e == T{}) continue;
            if (m_elements[idx].compare_exchange_strong(e, T{})) {
                //std::cout << "pop " << (i - search_start) << std::endl;
                search_start = i;
                if constexpr(keep_stats) { m_size++; }
                return true;
            }
        }
        //std::cout << "pop failed" << std::endl;
        return false;
    }

    /**
     * Quite inefficient. For debug purposes only.
     * @return
     */
    bool empty() {
        // NOTE: Not thread-safe
        if constexpr(keep_stats) {
            return m_size == 0;
        } else {
            // NOTE: Very inefficient
            for (size_t idx = 0; idx < m_capacity; ++idx) {
                if (m_elements[idx].load() != T{}) {
                    return false;
                }
            }
            return true;
        }
    }

    void reset() {
        // NOTE: Not thread-safe
        for (size_t idx = 0; idx < m_capacity; ++idx) {
            m_elements[idx] = T{};
        }
    }

    size_t size() {
        // NOTE: Not thread-safe
        if constexpr(keep_stats) {
            return m_size.load();
        } else {
            throw std::logic_error("");
        }

    }

private:

    std::unique_ptr<std::atomic<T>[]> m_elements;
    size_t m_capacity;

    std::mt19937 m_generator;
    std::mutex m_generator_mutex;

    // NOTE: only for keep_stats
    std::atomic<size_t> m_size{0};

    size_t thread_offset(bool set_value, size_t new_offset) {
        thread_local static size_t offset;
        thread_local static size_t offset_initialized;

        if (set_value) {
            offset = new_offset;
            return offset;
        }

        // NOTE: This happens at most once per thread.
        if (!offset_initialized) {
            std::scoped_lock lock(m_generator_mutex);
            std::uniform_int_distribution<size_t> dist;
            offset = dist(m_generator);
            offset_initialized = true;
        }
        return offset;
    }
};
