
#ifndef EXERCISE5_ATOMIC_SHARED_PTR_H
#define EXERCISE5_ATOMIC_SHARED_PTR_H

#include <atomic>

namespace epcpp::atomic {
    namespace detail {
        template<class T>
        struct Block {
            ~Block() {
                assert(m_ref_count.load() == 0);
            }

            T m_obj;
            std::atomic<long> m_ref_count;
        };
    }

    template<class T>
    class shared_ptr {
    public:
        using element_type = std::remove_extent_t<T>;

        constexpr shared_ptr() noexcept = default;

        constexpr shared_ptr(std::nullptr_t) noexcept: shared_ptr() {};

        shared_ptr(const shared_ptr &r) noexcept {
            assert(m_ptr == nullptr);
            if (r.m_ptr) {
                r.m_ptr->m_ref_count.fetch_add(1);
                m_ptr = r.m_ptr;
            }
            assert(m_ptr == r.m_ptr);
        }

        shared_ptr(shared_ptr &&r) noexcept {
            assert(m_ptr == nullptr);
            m_ptr = std::exchange(r.m_ptr, nullptr);
        }

        ~shared_ptr() {
            reset();
        }

        shared_ptr &operator=(std::nullptr_t) noexcept {
            reset();
        }

        shared_ptr &operator=(const shared_ptr &r) noexcept {
            if (&r == this) return *this;
            reset();
            if (r.m_ptr) {
                r.m_ptr->m_ref_count.fetch_add(1);
                m_ptr = r.m_ptr;
            }
            assert(m_ptr == r.m_ptr);
            return *this;
        }

        shared_ptr &operator=(shared_ptr &&r) noexcept {
            reset();
            m_ptr = std::exchange(r.m_ptr, nullptr);
            return *this;
        }

        void reset() noexcept {
            if (m_ptr && m_ptr->m_ref_count.fetch_sub(1) == 1) {
                delete m_ptr;
            }
            m_ptr = nullptr;
        }

        T *get() const noexcept {
            return m_ptr ? &m_ptr->m_obj : nullptr;
        }

        T &operator*() const noexcept {
            assert(m_ptr);
            return *get();
        }

        T *operator->() const noexcept {
            assert(m_ptr);
            return get();
        }

        [[nodiscard]] long use_count() const noexcept { return m_ptr ? m_ptr->m_ref_count.load() : 0; }

        explicit operator bool() const noexcept { return m_ptr != nullptr; }

        template<class ...Args>
        static shared_ptr<T> make_shared(Args &&... args) {
            auto *ptr = new detail::Block<T>{T(std::forward<Args>(args)...), 1};
            return shared_ptr<T>::from_raw(ptr);
        }

    protected:

        /**
         * Claims ptr as new shared_ptr without affecting the ref_count.
         * @param ptr
         * @return
         */
        static shared_ptr from_raw(detail::Block<T> *ptr) {
            shared_ptr result;
            result.m_ptr = ptr;
            return result;
        }

        /**
         * Releases m_ptr of r without affecting the ref_count.
         * @param r
         * @return
         */
        static detail::Block<T> *into_raw(shared_ptr &&r) {
            return std::exchange(r.m_ptr, nullptr);
        }

    private:

        detail::Block<T> *m_ptr{nullptr};

        template<class U>
        friend
        class atomic;
    };

    template<class T>
    class atomic;

    template<class T>
    class atomic<shared_ptr<T>> {
    public:
        using value_type = shared_ptr<T>;

        static constexpr bool is_always_lock_free = std::atomic<detail::Block<T> *>::is_always_lock_free;

        constexpr atomic() noexcept = default;

        explicit atomic(shared_ptr<T> desired) noexcept {
            m_ptr.store(shared_ptr<T>::into_raw(std::move(desired)));
        }

        atomic(const atomic &) = delete;

        void operator=(const atomic &) = delete;

        void operator=(shared_ptr<T> desired) noexcept {
            exchange(std::move(desired));
        }

        void store(shared_ptr<T> desired, std::memory_order order = std::memory_order_seq_cst) noexcept {
            auto *desired_ptr = std::exchange(desired.m_ptr, nullptr);
            auto *old_ptr = m_ptr.exchange(desired_ptr, order);
            desired.m_ptr = old_ptr;
        }

        shared_ptr<T> load(std::memory_order order = std::memory_order_seq_cst) const noexcept {
            auto *ptr = m_ptr.load(order);
            if (ptr) ptr->m_ref_count.fetch_add(1);
            return shared_ptr<T>::from_raw(ptr);
        }

        operator shared_ptr<T>() const noexcept {
            return load();
        }

        shared_ptr<T>
        exchange(shared_ptr<T> desired, std::memory_order order = std::memory_order_seq_cst) noexcept {
            // nullptr -> desired -> *this -> result
            auto *desired_ptr = shared_ptr<T>::into_raw(std::move(desired));
            auto *previous_ptr = m_ptr.exchange(desired_ptr, order);
            return shared_ptr<T>::from_raw(previous_ptr);
        }

        bool compare_exchange_strong(shared_ptr<T> &expected, shared_ptr<T> desired, std::memory_order success,
                                     std::memory_order failure) noexcept {
            auto *old_expected_ptr = expected.m_ptr;
            // (a, b, c) = (*this, expected, desired)
            if (m_ptr.compare_exchange_strong(expected.m_ptr, desired.m_ptr, success, failure)) {
                //  a == b
                //  (a/b, a/b, c) => (c, a/b, c)
                //  a' = c, b' = a/b, c' = c
                if (desired.m_ptr) desired.m_ptr->m_ref_count.fetch_add(1);          // a'/c_ref++
                if (expected.m_ptr) {
                    auto prev_ref_count = expected.m_ptr->m_ref_count.fetch_sub(1);  // b'/a_ref--
                    // initially, both `*this` and `expected` pointed at the same block. Now only `expected` does
                    assert(prev_ref_count > 1);
                }
                return true;
            } else {
                //  a != b
                //  (a, b, c) => (a, a, c)
                //  a' = a, b' = a, c' = c
                if (expected.m_ptr) expected.m_ptr->m_ref_count.fetch_add(1);          // b'/a_ref++
                if (old_expected_ptr) {
                    auto prev_ref_count = old_expected_ptr->m_ref_count.fetch_sub(1);  // b_ref--
                    // `expected` might have had the only pointer to the `b` block
                    if (prev_ref_count == 1) {
                        // The block is freed in the destructor
                        shared_ptr<T>::from_raw(old_expected_ptr);
                    }
                }
                return false;
            }
        }

        bool compare_exchange_strong(shared_ptr<T> &expected, shared_ptr<T> desired,
                                     std::memory_order order = std::memory_order_seq_cst) noexcept {
            return compare_exchange_strong(expected, std::move(desired), order, order);
        }

        bool compare_exchange_weak(shared_ptr<T> &expected, shared_ptr<T> desired, std::memory_order success,
                                   std::memory_order failure) noexcept {
            auto *old_expected_ptr = expected.m_ptr;
            // (a, b, c) = (*this, expected, desired)
            if (m_ptr.compare_exchange_weak(expected.m_ptr, desired.m_ptr, success, failure)) {
                //  a == b
                //  (a/b, a/b, c) => (c, a/b, c)
                //  a' = c, b' = a/b, c' = c
                if (desired.m_ptr) desired.m_ptr->m_ref_count.fetch_add(1);          // a'/c_ref++
                if (expected.m_ptr) {
                    auto prev_ref_count = expected.m_ptr->m_ref_count.fetch_sub(1);  // b'/a_ref--
                    // initially, both `*this` and `expected` pointed at the same block. Now only `expected` does
                    assert(prev_ref_count > 1);
                }
                return true;
            } else {
                //  a != b
                //  (a, b, c) => (a, a, c)
                //  a' = a, b' = a, c' = c
                if (expected.m_ptr) expected.m_ptr->m_ref_count.fetch_add(1);          // b'/a_ref++
                if (old_expected_ptr) {
                    auto prev_ref_count = old_expected_ptr->m_ref_count.fetch_sub(1);  // b_ref--
                    // `expected` might have had the only pointer to the `b` block
                    if (prev_ref_count == 1) {
                        // The block is freed in the destructor
                        shared_ptr<T>::from_raw(old_expected_ptr);
                    }
                }
                return false;
            }
        }

        bool compare_exchange_weak(shared_ptr<T> &expected, shared_ptr<T> desired,
                                   std::memory_order order = std::memory_order_seq_cst) noexcept {
            return compare_exchange_weak(expected, std::move(desired), order, order);
        }

    private:
        std::atomic<detail::Block<T> *> m_ptr;
    };

}

#endif //EXERCISE5_ATOMIC_SHARED_PTR_H
