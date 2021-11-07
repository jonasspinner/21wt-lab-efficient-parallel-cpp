#pragma once

/*******************************************************************************
*** implementation of a linear probing hash table                            ***
*** for subexercise b) you can optimize performance by changing the layout.  ***
*** you may change any function leaving the overall functionality intact.    ***
*******************************************************************************/

#include <cstddef>
#include <utility>
#include <vector>
#include <cassert>

#include "cell.h"
#include "cell_iterator.h"

template<class K, class D, class HF>
class HashB {
private:
    using size_t = std::size_t;
    using KeyType = K;
    using DataType = D;
    using PairType = std::pair<K, D>;
    using CellType = Cell<K, D>;
    using IteratorType = CellIterator<CellType>;
    using ConstIteratorType = CellIterator<CellType, true>;
    using InsertReturnType = std::pair<IteratorType, bool>;
    using HashFunction = HF;

    static constexpr size_t max_search_length = 400;

public:
#ifdef USE_STD_VECTOR
    explicit HashB(size_t size)
            : table(static_cast<size_t>(1) << static_cast<int>(std::ceil(std::log2(static_cast<double>(size) * 1.3)))) {}
#else
    explicit HashB(size_t size)
            : m_capacity(static_cast<size_t>(1) << static_cast<int>(std::ceil(std::log2(static_cast<double>(size) * 1.3)))) {
        assert(is_power_of_two(capacity()));
        const auto alignment = std::max<size_t>(alignof(CellType), 64);
        const auto alloc_size = sizeof(CellType) * capacity();
        table = static_cast<CellType*>(aligned_alloc(alignment, alloc_size));
        std::uninitialized_fill_n(table, capacity(), CellType());
    }

    ~HashB() {
        for (size_t i = 0; i < capacity(); ++i) {
            table[i].~CellType();
        }
        free(table);
        table = nullptr;
        m_capacity = 0;
    }
#endif

    InsertReturnType insert(PairType p) {
        size_t hash_pos = map(p.first);
        auto mask = (capacity() - 1);

        for (size_t i = hash_pos; i < hash_pos + max_search_length; ++i) {
            size_t cur_pos = i & mask; // % (1 << log_capacity);
            const auto &cur_element = table[cur_pos];

            if (cur_element.compareKey(p.first)) {
                return makeInsertRet(cur_pos, false);
            } else if (cur_element.isEmpty()) {
                table[cur_pos] = CellType(std::move(p));
                return makeInsertRet(cur_pos, true);
            }
        }
        return InsertReturnType(IteratorType(), false);
    }

private:
    // returns the position, if k is present otherwise returns capacity
    size_t findPos(const KeyType &k) const {
        size_t hash_pos = map(k);
        auto mask = capacity() - 1;

        for (size_t i = hash_pos; i < hash_pos + max_search_length; ++i) {
            size_t cur_pos = i & mask; // % (1 << log_capacity);
            const auto &cur_element = table[cur_pos];

            if (cur_element.compareKey(k)) {
                return cur_pos;
            } else if (cur_element.isEmpty()) {
                return capacity();
            }
        }
        return capacity();
    }

public:
    /* both find implementations use the above findPos function ***************/
    IteratorType find(const KeyType &k) {
        auto pos = findPos(k);
        return (pos < capacity()) ? makeIterator(pos) : IteratorType();
    }

    ConstIteratorType find(const KeyType &k) const {
        auto pos = findPos(k);
        return (pos < capacity()) ? makeCIterator(pos) : ConstIteratorType();
    }

private:
    /* member definitions *****************************************************/
#ifdef USE_STD_VECTOR
    std::vector<CellType> table;
#else
    std::size_t m_capacity;
    CellType* table;
#endif
    HashFunction hash_function;

    /* some utility functions *************************************************/
    size_t capacity() const {
#ifdef USE_STD_VECTOR
        return table.size();
#else
        return m_capacity;
#endif
    }

    [[nodiscard]] const CellType* table_end() const {
#ifdef USE_STD_VECTOR
        return &table.back() + 1;
#else
        return table + capacity();
#endif
    }

    [[nodiscard]] size_t map(const KeyType &k) const {
        size_t hashed = hash_function(k);
        return hashed & (capacity() - 1); // % (1 << log_capacity);
    }

    IteratorType makeIterator(size_t pos) { return IteratorType(&table[pos], table_end()); }

    ConstIteratorType makeCIterator(size_t pos) const { return ConstIteratorType(&table[pos], table_end()); }

    InsertReturnType makeInsertRet(size_t pos, bool succ) { return std::make_pair(makeIterator(pos), succ); }

    static bool is_power_of_two(std::size_t value) {
        return (value & (value - 1)) == 0;
    }

public:

    /***************************************************************************
    *** hash table interface: CAN PROBABLY REMAIN UNCHANGED ********************
    ***************************************************************************/

    /* typedefs similar to std::unordered_map *********************************/
    using key_type = KeyType;
    using mapped_type = DataType;
    using value_type = std::pair<KeyType, DataType>;
    using iterator = IteratorType;
    using const_iterator = ConstIteratorType;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using insert_return_type = std::pair<iterator, bool>;

    /* iterator functions *****************************************************/
    iterator begin() {
        auto it = makeIterator(0);
        if (CellType(*it).isEmpty()) ++it;
        return it;
    }

    const_iterator begin() const { return cbegin(); }

    const_iterator cbegin() const {
        auto it = makeCIterator(0);
        if (CellType(*it).isEmpty()) ++it;
        return it;
    }

    iterator end() { return iterator(); }

    const_iterator end() const { return cend(); }

    const_iterator cend() const { return const_iterator(); }

    /* accessor functions *****************************************************/
    mapped_type &at(const key_type &k) {
        auto it = find(k);
        if (it == end()) throw std::out_of_range("cannot find key");
        return (*it).second;
    }

    const mapped_type &at(const key_type &k) const {
        auto it = find(k);
        if (it == end()) throw std::out_of_range("cannot find key");
        return (*it).second;
    }

    mapped_type &operator[](const key_type &k) {
        auto it = insert(k).first;
        return (*it).second;
    }

    size_type count(const key_type &k) const {
        return (find(k) != end()) ? 1 : 0;
    }
};
