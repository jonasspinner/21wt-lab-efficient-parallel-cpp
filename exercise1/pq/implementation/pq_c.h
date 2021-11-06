#pragma once

#include <queue>
#include <utility>
#include <cassert>

#include "AlignedVector.h"

template<class T, class Comp = std::less<T> >
class PriQueueC {
public:
    explicit PriQueueC(std::size_t capacity, std::size_t log_degree = 3)
            : m_elements(capacity, static_cast<size_t>(1) << log_degree, (static_cast<size_t>(1) << log_degree) - 1),
              m_log_degree(log_degree) {}

    const T &top() const {
        assert(!empty());
        return m_elements.front();
    }

    [[nodiscard]] bool empty() const { return m_elements.empty(); }

    [[nodiscard]] std::size_t size() const { return m_elements.size(); }

    void push(T value) {
        m_elements.push_back(value);

        size_t i = size() - 1;
        while (i > 0) {
            auto p = parent(i);
            if (m_comp(m_elements[i], m_elements[p])) {
                break;
            } else {
                std::swap(m_elements[p], m_elements[i]);
                assert(m_comp(m_elements[i], m_elements[parent(i)]));
                i = p;
            }
        }
    }

    void pop() {
        auto const degree = static_cast<size_t>(1) << m_log_degree;

        assert(!empty());

        m_elements[0] = std::move(m_elements.back());
        m_elements.pop_back();

        if (size() == 0)
            return;

        size_t i = 0;
        while (true) {
            assert(i < size());

            size_t max_idx = i;
            for (size_t k = child(i, 0); k < std::min(child(i, degree - 1) + 1, size()); ++k) {
                if (m_comp(m_elements[max_idx], m_elements[k])) {
                    max_idx = k;
                }
            }
            if (i == max_idx)
                break;
            std::swap(m_elements[i], m_elements[max_idx]);

            i = max_idx;
        }
    }

private:
    [[nodiscard]] constexpr size_t child(size_t i, size_t j) const {
        auto const degree = static_cast<size_t>(1) << m_log_degree;
        return i * degree + j + 1;
    }

    [[nodiscard]] constexpr size_t parent(size_t i) const {
        return (i - 1) >> m_log_degree;
    }

    [[nodiscard]] bool is_valid() const {
        for (size_t i = 1; i < size(); ++i) {
            if (!m_comp(m_elements[i], m_elements[parent(i)])) {
                return false;
            }
        }
        return true;
    }

private:
    /* member definitions *****************************************************/
    AlignedVector<T> m_elements;
    std::size_t m_log_degree;
    Comp m_comp{};
};
