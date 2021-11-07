#pragma once

#include<utility>

template<class K, class D>
class Cell
{
public:
    using value_type = std::pair<const K, D>;

    Cell()                        : pair()     { }
    Cell(const K& k, const D& d)  : pair(k, d) { }
    Cell(const std::pair<K,D>& p) : pair(p)    { }
    Cell(const Cell& rhs)            = default;
    Cell& operator=(const Cell& rhs) = default;
    Cell(Cell&& rhs)                 = default;
    Cell& operator=(Cell&& rhs)      = default;

#ifdef HASH_B
    explicit Cell(std::pair<K, D> &&p) : pair(std::move(p)) {}
#endif
#ifdef HASH_C
    explicit Cell(std::pair<K, D> &&p) : pair(std::move(p)) {}
#endif

    ~Cell() = default;

    bool isEmpty() const
    {
        return pair.first == K();
    }

    bool compareKey(const K& k) const
    {
        return pair.first == k;
    }

    operator value_type() const { return pair; }
    operator value_type()       { return pair; }

private:
    std::pair<K,D> pair;
};
