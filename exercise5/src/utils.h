
#ifndef EXERCISE5_UTILS_H
#define EXERCISE5_UTILS_H

#include <cstddef>
#include <bit>
#include <memory>

namespace epcpp::utils {
    constexpr std::size_t next_power_of_two(std::size_t n) {
        std::size_t power = 1;
        while (power < n) {
            power <<= 1;
        }
        return power;
    }

    constexpr bool is_power_of_two(std::size_t n) {
        return (n & (n - 1)) == 0;
    }

    double to_ms(std::chrono::nanoseconds time) {
        return (double) time.count() / 1e6;
    }

    namespace debug_allocator {
        struct Counts {
            std::atomic<std::size_t> num_allocate{};
            std::atomic<std::size_t> num_deallocate{};
        };

        template<class T>
        class CountingAllocator {
        public:
            using value_type = T;
            using size_type = std::size_t;
            using difference_type = std::ptrdiff_t;
            using propagate_on_container_move_assignment = std::true_type;

            constexpr CountingAllocator() : m_counts(std::make_shared<Counts>()) {}

            constexpr CountingAllocator(const CountingAllocator &other) noexcept
                    : m_allocator(other.m_allocator), m_counts(other.m_counts) {}

            template<class U>
            constexpr explicit CountingAllocator(const CountingAllocator<U> &other) noexcept
                    : m_allocator(other.m_allocator), m_counts(other.m_counts) {}

            [[nodiscard]] constexpr T *allocate(std::size_t n) {
                m_counts->num_allocate.fetch_add(1, std::memory_order_relaxed);
                return m_allocator.allocate(n);
            }

            constexpr void deallocate(T *p, std::size_t n) {
                m_counts->num_deallocate.fetch_add(1, std::memory_order_relaxed);
                m_allocator.deallocate(p, n);
            }

            template<class U>
            struct rebind {
                typedef CountingAllocator<U> other;
            };

            [[nodiscard]] const Counts &counts() const { return *m_counts; }

            friend std::ostream &operator<<(std::ostream &os, const CountingAllocator &alloc) {
                return os << alloc.m_counts->num_allocate << " " << alloc.m_counts->num_deallocate;
            }

        private:
            template<class U>
            friend
            class CountingAllocator;

            std::allocator<T> m_allocator;
            std::shared_ptr<Counts> m_counts;
        };
    }

    struct MurmurHashWrapper {
        template<class T>
        std::size_t operator()(const T &value) const {
            auto x = std::hash<T>{}(value);
            // murmurhash3
            // https://github.com/martinus/robin-hood-hashing/blob/master/src/include/robin_hood.h#L748-L759
            x ^= x >> 33U;
            x *= UINT64_C(0xff51afd7ed558ccd);
            x ^= x >> 33U;
            //x *= UINT64_C(0xc4ceb9fe1a85ec53);
            //x ^= x >> 33U;
            return static_cast<size_t>(x);
        }
    };
}

#endif //EXERCISE5_UTILS_H
