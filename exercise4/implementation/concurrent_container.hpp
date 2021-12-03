#pragma once

#include <cstddef>
#include <memory>

template <class T>
class ConcurrentContainer
{
public:
    ConcurrentContainer(std::size_t capacity)
    {
    }

    // fully concurrent
    void push(const T& e);      // TODO
    // fully concurrent
    T    pop();                // TODO

    // Whatever other functions you want

private:
};
