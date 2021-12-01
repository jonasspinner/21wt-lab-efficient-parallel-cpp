#pragma once

#include <cstddef>
#include <memory>

template <class T>
class ConcurrentQueue
{
public:
    ConcurrentQueue(std::size_t capacity)
    {
        _array = std::make_unique<T[]>(capacity, T());
    }

    // fully concurrent
    void push(const T& e);      // i.e. push_back(e) TODO
    // fully concurrent
    T    pop();                // i.e. pop_front() TODO

    // Whatever other functions you want

private:
    std::unique_ptr<T[]> _array;
};
