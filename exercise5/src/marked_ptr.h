#ifndef EXERCISE5_MARKED_PTR_H
#define EXERCISE5_MARKED_PTR_H

namespace epcpp {
    template<class T>
    class marked_ptr {
        static_assert(sizeof(T *) == sizeof(std::size_t));
    public:
        constexpr marked_ptr() = default;

        constexpr marked_ptr(T *ptr) : m_ptr(ptr) {
            assert(!is_marked());
        }

        constexpr marked_ptr(const marked_ptr &other) = default;

        [[nodiscard]] constexpr bool is_marked() const {
            return ((std::size_t) m_ptr) & mask;
        }

        [[nodiscard]] constexpr marked_ptr as_marked() const {
            assert(!is_marked());
            marked_ptr result = *this;
            result.m_ptr = (T *) (((std::size_t) result.m_ptr) | mask);
            return result;
        }

        [[nodiscard]] constexpr marked_ptr as_unmarked() const {
            return marked_ptr(get_unmarked());
        }

        [[nodiscard]] constexpr T *get() const {
            assert(!is_marked());
            return m_ptr;
        }

        T &operator*() const { return *get_unmarked(); }

        T *operator->() const { return get_unmarked(); }

        /*
        explicit operator T *() const { return get_unmarked(); }

        */
        explicit operator bool() const { return get_unmarked() != nullptr; }


        [[nodiscard]] constexpr T *get_unmarked() const {
            return (T *) (((std::size_t) m_ptr) & ~mask);
        }


    private:
        static constexpr std::size_t mask = ((std::size_t) 1) << 63;
        T *m_ptr{nullptr};
    };
}

#endif //EXERCISE5_MARKED_PTR_H
