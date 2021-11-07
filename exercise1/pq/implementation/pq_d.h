#pragma once

#include <queue>
#include <utility>

#include "AlignedVector.h"

template<class T, class Comp = std::less<T> >
class PriQueueD {
public:
    struct handle {
        std::size_t idx;
    };
private:
    struct Element {
        T e;
        handle h;
    };
public:
    explicit PriQueueD(std::size_t capacity, std::size_t log_degree = 3)
            : m_elements(capacity, static_cast<size_t>(1) << log_degree, (static_cast<size_t>(1) << log_degree) - 1),
              m_log_degree(log_degree) {}

    const T &top() const {
        assert(!empty());
        return m_elements.front().e;
    }

    [[nodiscard]] bool empty() const { return m_elements.empty(); }

    [[nodiscard]] std::size_t size() const { return m_elements.size(); }

    handle push(T value) {
        m_positions.push_back(m_elements.size());
        handle h = {m_positions.size() - 1};
        m_elements.push_back({std::move(value), h});

        assert(pos(m_elements[size() - 1].h) == size() - 1);

        fix_upwards(size() - 1);

        return h;
    }

private:
    void fix_upwards(std::size_t i) {
        assert(i < size());
        while (i > 0) {
            const auto p = parent(i);
            if (!m_comp(m_elements[p].e, m_elements[i].e)) {
                break;
            } else {
                std::swap(pos(m_elements[p].h), pos(m_elements[i].h));
                std::swap(m_elements[p], m_elements[i]);
                assert(pos(m_elements[p].h) == p);
                assert(pos(m_elements[i].h) == i);

                assert(!m_comp(m_elements[p].e, m_elements[i].e));
                i = p;
            }
        }
    }

public:
    void pop() {
        assert(!empty());

        pos(m_elements[0].h) = std::numeric_limits<std::size_t>::max();
        m_elements[0] = std::move(m_elements.back());
        pos(m_elements[0].h) = 0;
        assert(pos(m_elements[0].h) == 0);
        m_elements.pop_back();

        if (size() == 0)
            return;

        fix_downwards(0);
    }

private:
    void fix_downwards(std::size_t i) {
        assert(i < size());
        auto const degree = static_cast<size_t>(1) << m_log_degree;

        while (true) {
            assert(i < size());

            size_t max_idx = i;
            for (size_t k = child(i, 0); k < std::min(child(i, degree - 1) + 1, size()); ++k) {
                if (m_comp(m_elements[max_idx].e, m_elements[k].e)) {
                    max_idx = k;
                }
            }
            if (i == max_idx)
                break;
            std::swap(pos(m_elements[i].h), pos(m_elements[max_idx].h));
            std::swap(m_elements[i], m_elements[max_idx]);
            assert(pos(m_elements[i].h) == i);
            assert(pos(m_elements[max_idx].h) == max_idx);

            i = max_idx;
        }
    }

public:
    const T &get_key(handle h) const {
        return m_elements[pos(h)].e;
    }

    // implement a log(n) key change function
    void change_key(handle h, T newvalue) {
        const auto i = pos(h);
        if (m_comp(m_elements[i].e, newvalue)) {
            m_elements[i].e = std::move(newvalue);
            fix_upwards(i);
        } else {
            m_elements[i].e = std::move(newvalue);
            fix_downwards(i);
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
            if (m_comp(m_elements[parent(i)], m_elements[i])) {
                return false;
            }
        }
        return true;
    }

    std::size_t &pos(handle h) {
        return m_positions[h.idx];
    }

    std::size_t pos(handle h) const {
        return m_positions[h.idx];
    }

private:
    /* member definitions *****************************************************/
    AlignedVector<Element> m_elements;
    std::vector<std::size_t> m_positions;
    std::size_t m_log_degree;
    Comp m_comp{};
    // T* or std::vector<T>
    // some data-structure for handles:
    //   each handle should store the heap position, thus it has to be updated
    //   with each move in the heap
};
