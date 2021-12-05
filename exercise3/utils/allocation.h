#ifndef ALLOCATION_H
#define ALLOCATION_H

#include <cstdlib>
#include <algorithm>
#include <cassert>


namespace allocation {
    [[nodiscard]] static constexpr std::size_t next_power_of_two(std::size_t value) noexcept {
        if (value <= 1) {
            return 1;
        }
        std::size_t k = 2;
        value--;
        while (value >>= 1) {
            k <<= 1;
        }
        return k;
    }

    [[nodiscard]] static constexpr bool is_power_of_two(std::size_t value) noexcept {
        return (value & (value - 1)) == 0;
    }

    [[nodiscard]] static constexpr bool is_multiple_of(std::size_t value, std::size_t multiple) noexcept {
        return value == (value / multiple) * multiple;
    }

    template<class T>
    static T *allocate_at_least(std::size_t size) {
        static_assert(next_power_of_two(64) == 64);

        const auto alignment = next_power_of_two(std::max<std::size_t>(alignof(T), 1024));
        const auto size_bytes = ((sizeof(T) * size + alignment - 1) / alignment) * alignment;

        assert(is_power_of_two(alignment));
        assert(is_multiple_of(size_bytes, alignment));
        assert(size_bytes >= sizeof(T) * size);

        return static_cast<T *>(std::aligned_alloc(alignment, size_bytes));
    }
}

#endif //ALLOCATION_H
