#pragma once

#include <queue>
#include <utility>

template<class T, class Comp = std::less<T> >
class PriQueueC
{
public:
    PriQueueC(std::size_t capacity, std::size_t log_degree = 3) { }

    const T&    top()   const { return pq.top();   }
    bool        empty() const { return pq.empty(); }
    std::size_t size()  const { return pq.size();  }

    void push(T value) { return pq.push(std::move(value)); }
    void pop ()        { return pq.pop();       }

private:
    /* member definitions *****************************************************/
    std::priority_queue<T, std::vector<T>, Comp> pq;
    // T* or std::vector<T>
};
