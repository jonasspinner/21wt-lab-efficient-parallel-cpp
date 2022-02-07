
#ifndef EXERCISE5_UTILS_H
#define EXERCISE5_UTILS_H

#include <cstddef>
#include <bit>

namespace epcpp {
    namespace utils {
        constexpr std::size_t next_power_of_two(std::size_t n) {
            std::size_t power = 1;
            while (n < power) {
                power <<= 1;
            }
            return power;
        }

        constexpr bool is_power_of_two(std::size_t n) {
            return (n & (n - 1)) == 0;
        }

        double to_ms(std::chrono::nanoseconds time) {
            return (double)time.count() / 1e6;
        }
    }
}

#endif //EXERCISE5_UTILS_H
