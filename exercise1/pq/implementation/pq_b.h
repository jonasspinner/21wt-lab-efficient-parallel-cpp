#pragma once

#include <queue>
#include <utility>
#include <cassert>
#include <algorithm>

#include "AlignedVector.h"

template<class T, std::size_t degree = 8, class Comp = std::less<T> >
class PriQueueB {
public:
    explicit PriQueueB(std::size_t capacity) : m_elements(capacity, degree, degree - 1) {}

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

        auto* const elements = m_elements.data();

        while (true) {
            assert(i < size());

            auto* max_ptr = elements + i;

            auto* const begin = elements + child(i, 0);
            if (begin + degree <= elements + size()) {
                for (auto* k = begin; k < begin + degree; ++k) {
                    if (m_comp(*max_ptr, *k)) {
                        max_ptr = k;
                    }
                }
            } else {
                break;
            }

            if (max_ptr == elements + i) {
                return;
            }
            std::swap(elements[i], *max_ptr);

            i = static_cast<size_t>(std::distance(elements, max_ptr));
        }

        auto* const begin = elements + child(i, 0);
        auto* const end = std::min(begin + degree, elements + size());
        if (begin < end) {
            auto* max_ptr = std::max_element(begin, end, m_comp);
            if (m_comp(elements[i], *max_ptr)) {
                std::swap(elements[i], *max_ptr);
            }
        }
    }

private:
    [[nodiscard]] constexpr size_t child(size_t i, size_t j) const {
        return i * degree + j + 1;
    }

    [[nodiscard]] constexpr size_t parent(size_t i) const {
        return (i - 1) / degree;
    }

    [[nodiscard]] bool is_valid() const {
        for (size_t i = 1; i < size(); ++i) {
            if (m_comp(m_elements[parent(i)], m_elements[i])) {
                return false;
            }
        }
        return true;
    }

    /* member definitions *****************************************************/
    AlignedVector<T> m_elements;
    Comp m_comp{};
};
