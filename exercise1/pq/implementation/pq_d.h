#pragma once

#include <queue>
#include <utility>

template<class T, class Comp = std::less<T> >
class PriQueueD
{
public:
    PriQueueD(std::size_t capacity, std::size_t log_degree = 3) { }

public:
    using handle = size_t; // this is only an placeholder, could be a pointer to
                           // an indirectly stored data object, or something
                           // similar

    const T&    top()   const { return pq.top();   }
    bool        empty() const { return pq.empty(); }
    std::size_t size()  const { return pq.size();  }

    handle   push(const T& value) { pq.push(value); return 0; }
    void     pop ()        { pq.pop(); }

    const T& get_key(handle h) const {
        // TODO
        return top();
    }

    // implement a log(n) key change function
    void change_key(handle h, const T& newvalue)
    {   /* ----- TODO ----- */   }

private:
    /* member definitions *****************************************************/
    std::priority_queue<T, std::vector<T>, Comp> pq;
    // T* or std::vector<T>
    // some data-structure for handles:
    //   each handle should store the heap position, thus it has to be updated
    //   with each move in the heap
};
