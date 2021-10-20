#pragma once

#include<type_traits>
#include<iterator>

template<class C, bool is_const = false>
class CellIterator
{
public:
    using CellType = typename std::conditional<is_const, const C, C>::type;
    using VT       = typename C::value_type;

    friend CellIterator<C,!is_const>;
public:
/* Iterator Interface: Necessary Typedefs *************************************/

    using difference_type = std::ptrdiff_t;
    using value_type      = typename std::conditional<is_const, const VT, VT>::type;
    using reference       = value_type&;
    using pointer         = value_type*;
    using iterator_category = std::forward_iterator_tag;

    template<class C_, bool ic>
    friend void swap(CellIterator<C_,ic>& l, CellIterator<C_,ic>& r);


/* Constructors ***************************************************************/

    CellIterator(CellType* _ptr = nullptr, const CellType* _eptr = nullptr)
        : ptr(_ptr), eptr(_eptr) { }
    CellIterator(const CellIterator& rhs)
        : ptr(rhs.ptr), eptr(rhs.eptr) { }
    ~CellIterator() = default;


/* Iterator Interface: Necessary Functionality ********************************/

    CellIterator& operator++(int) { return ++(*this); }
    CellIterator& operator++()
    {
        do
        {
            ptr++;
            if (ptr >= eptr) { ptr = nullptr; eptr = nullptr; return *this; }
        } while (ptr->isEmpty());

        return *this;
    }

    reference operator* () const { return *reinterpret_cast<value_type*>(ptr); }
    pointer   operator->() const { return reinterpret_cast<value_type*>(ptr);  }

    template<bool ic>
    bool operator==(const CellIterator<C,ic>& rhs) const { return ptr == rhs.ptr; }
    template<bool ic>
    bool operator!=(const CellIterator<C,ic>& rhs) const { return ptr != rhs.ptr; }

private:
    CellType* ptr;
    const CellType* eptr;
};
