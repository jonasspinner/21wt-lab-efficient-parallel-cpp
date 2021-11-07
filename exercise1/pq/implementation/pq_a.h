#pragma once

#include <queue>
#include <utility>
#include <cassert>
#include <algorithm>

#include "AlignedVector.h"

template<class T, class Comp = std::less<T> >
class PriQueueA {
public:
    explicit PriQueueA(std::size_t capacity, std::size_t degree = 8)
            : m_elements(capacity, degree, degree - 1), m_degree(degree) {}

    const T &top() const {
        assert(!empty());
        return m_elements.front();
    }

    [[nodiscard]] bool empty() const { return m_elements.empty(); }

    [[nodiscard]] std::size_t size() const { return m_elements.size(); }

    void push(T value) {
        m_elements.push_back(std::move(value));
        fix_upwards(size() - 1);
    }

private:
    void fix_upwards(std::size_t i) {
        assert(i < size());
        while (i > 0) {
            const auto p = parent(i);
            if (!m_comp(m_elements[p], m_elements[i])) {
                break;
            } else {
                std::swap(m_elements[p], m_elements[i]);
                assert(!m_comp(m_elements[p], m_elements[i]));
                i = p;
            }
        }
    }

public:
    void pop() {
        assert(!empty());

        m_elements[0] = std::move(m_elements.back());
        m_elements.pop_back();

        if (size() == 0)
            return;

        fix_downwards(0);
    }

private:
    void fix_downwards(std::size_t i) {
        assert(i < size());

        while (true) {
            assert(i < size());

            size_t max_idx = i;
            for (size_t k = child(i, 0); k < std::min(child(i, m_degree - 1) + 1, size()); ++k) {
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
        return i * m_degree + j + 1;
    }

    [[nodiscard]] constexpr size_t parent(size_t i) const {
        return (i - 1) / m_degree;
    }

    [[nodiscard]] bool is_valid() const {
        /*
        for (size_t i = 0; i < size(); ++i) {
            for (size_t k = child(i, 0); k < std::min(child(i, m_degree - 1) + 1, size()); ++k) {
                if (!m_comp(m_elements[i], m_elements[k]))
                    return false;
            }
        }
        */
        for (size_t i = 1; i < size(); ++i) {
            if (m_comp(m_elements[parent(i)], m_elements[i])) {
                return false;
            }
        }
        return true;
    }

    /* member definitions *****************************************************/
    AlignedVector<T> m_elements;
    const size_t m_degree;
    const Comp m_comp{};
};
