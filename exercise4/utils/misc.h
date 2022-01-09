#ifndef EXERCISE4_MISC_H
#define EXERCISE4_MISC_H

#include <utility>

namespace utils {
    template<class It>
    class Range {
    public:
        constexpr explicit Range(std::pair<It, It> pair) : m_begin(pair.first), m_end(pair.second) {}

        [[nodiscard]] constexpr It begin() const {
            return m_begin;
        }

        [[nodiscard]] constexpr It end() const {
            return m_end;
        }

        [[nodiscard]] constexpr auto size() const {
            return end() - begin();
        }

        [[nodiscard]] constexpr bool empty() const {
            return begin() == end();
        }

    private:
        It m_begin{};
        It m_end{};
    };

    auto to_ms(std::chrono::nanoseconds d) {
        return (double) d.count() / 1'000'000.0;
    }
}

#endif //EXERCISE4_MISC_H
