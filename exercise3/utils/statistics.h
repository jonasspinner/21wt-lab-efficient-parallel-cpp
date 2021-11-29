#ifndef STATISTICS_H
#define STATISTICS_H

#include <vector>
#include <numeric>
#include <cassert>

namespace statistics {
    template<class T>
    T mean(const std::vector<T> &xs) {
        return std::accumulate(xs.begin(), xs.end(), T{}) / static_cast<T>(xs.size());
    }

    template<class T>
    T standard_deviation(const std::vector<T> &xs) {
        assert(xs.size() >= 2);

        auto m = mean(xs);
        T s{};
        for (auto x: xs) {
            s += (x - m) * (x - m);
        }
        return std::sqrt(s / static_cast<T>(xs.size() - 1));
    }
}


#endif //STATISTICS_H
