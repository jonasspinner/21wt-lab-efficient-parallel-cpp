
#ifndef EXERCISE4_MUTEX_STD_QUEUE_HPP
#define EXERCISE4_MUTEX_STD_QUEUE_HPP

#include <queue>
#include <mutex>
#include <condition_variable>
#include <cassert>

template <class T>
class MutexStdQueue {
public:
    explicit MutexStdQueue([[maybe_unused]] std::size_t capacity) {}

    // fully concurrent
    void push(const T& e) {
        {
            std::scoped_lock lock(m_mutex);
            m_queue.push(e);
        }
        m_non_empty.notify_one();
    }

    // fully concurrent
    T    pop() {
        std::unique_lock lock(m_mutex);
        m_non_empty.wait(lock, [&]() { return !m_queue.empty(); });
        assert(lock);
        assert(!m_queue.empty());
        T e = m_queue.front();
        m_queue.pop();
        return e;
    }

    void reset() {
        std::scoped_lock lock(m_mutex);
        while (!m_queue.empty()) {
            m_queue.pop();
        }
    }

    [[nodiscard]] bool empty() {
        std::scoped_lock lock(m_mutex);
        return m_queue.empty();
    }

    [[nodiscard]] auto size() {
        std::scoped_lock lock(m_mutex);
        return m_queue.size();
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_non_empty;
    std::queue<T> m_queue;
};

#endif //EXERCISE4_MUTEX_STD_QUEUE_HPP
