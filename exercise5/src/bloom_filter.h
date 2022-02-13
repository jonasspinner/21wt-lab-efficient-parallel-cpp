#ifndef EXERCISE5_BLOOM_FILTER_H
#define EXERCISE5_BLOOM_FILTER_H

#include <iostream>

#include "bucket.h"

namespace epcpp {
    template<concepts::Bucket BucketBase, std::size_t NumFilters = 1>
    class BloomFilterAdapter {
        /**
         * from scipy.special import comb, factorial
         * import numpy as np
         * from functools import cache
         * https://rosettacode.org/wiki/Stirling_numbers_of_the_second_kind#Python
         * @cache
         * def sterling2(n, k):
         *
         * error = sum(i**k * factorial(i) * comb(m, i) * sterling2(k*n,i) / (m**(k*(n+1))) for i in range(1, m + 1))
         * k_opt = m / n * np.log(2)
         */
    public:
        using key_type = typename BucketBase::key_type;
        using mapped_type = typename BucketBase::mapped_type;
        using value_type = typename BucketBase::value_type;
        using key_equal = typename BucketBase::key_equal;

        using allocator_type = typename BucketBase::allocator_type;

        using handle = typename BucketBase::handle;
        using const_handle = typename BucketBase::const_handle;

        BloomFilterAdapter() {
            static_assert(concepts::Bucket<std::remove_reference_t<decltype(*this)>>);
        }

        explicit BloomFilterAdapter(const allocator_type &alloc) : m_bucket(alloc) {
            static_assert(concepts::Bucket<std::remove_reference_t<decltype(*this)>>);
        }

        std::pair<handle, bool> insert(value_type &&key_value_pair, std::size_t hash) {
            bloom_filter_insert(hash);

            return m_bucket.insert(std::move(key_value_pair), hash);
        }

        handle find(const key_type &key, std::size_t hash) {
            if (bloom_filter_contains(hash)) {
                return m_bucket.find(key, hash);
            }
            return end();
        }

        const_handle find(const key_type &key, std::size_t hash) const {
            if (bloom_filter_contains(hash)) {
                return m_bucket.find(key, hash);
            }
            return end();
        }

        bool erase(const key_type &key, std::size_t hash) {
            /*
            std::cout << "[bloom] remove(" << key << "," << hash << ") "
                      << (bloom_filter_contains(hash) ? "in" : "not in") << " bloom filter \t k=" << NumFilters << std::endl;
            std::cout << "\thash   " << std::bitset<64>(hash) << std::endl;
            std::cout << "\tfilter " << std::bitset<64>(m_bloom_filter) << std::endl;
            std::cout << "\tmask   " << std::bitset<64>(get_filter_mask(hash)) << std::endl;
            */
            if (bloom_filter_contains(hash)) {
                return m_bucket.erase(key, hash);
            }
            return false;
        }

        handle end() { return m_bucket.end(); }

        const_handle cend() const { return m_bucket.cend(); }

        const_handle end() const { return m_bucket.end(); }

        static std::string name() {
            std::stringstream ss;
            ss << "BloomFilter<" << BucketBase::name() << " " << NumFilters << ">";
            return ss.str();
        }

    private:
        void bloom_filter_insert(std::size_t hash) {
            m_bloom_filter.fetch_or(get_filter_mask(hash), std::memory_order_relaxed);
        }

        [[nodiscard]] bool bloom_filter_contains(std::size_t hash) const {
            const auto filter = m_bloom_filter.load(std::memory_order_relaxed);
            const auto mask = get_filter_mask(hash);
            return (filter & mask) == mask;
        }

        static std::uint64_t get_filter_mask(std::size_t hash) {
            // Use blocks of 6 bits, starting from the most significant bits, to get indices into the 64-bit filter.
            static_assert(std::numeric_limits<std::size_t>::digits == 64);
            constexpr std::uint64_t filter_width = 64;
            constexpr std::uint64_t mask_width = 6; // bits needed to index 64 positions, ceil(log2(filter_width))
            constexpr std::uint64_t mask = filter_width - 1; // mask_width of 1-bits in the least significant bits

            const auto hash_v = uint64_t{hash};

            std::uint64_t filter_mask{};

            static_assert(filter_width > NumFilters * mask_width);
            for (std::size_t i = 1; i <= NumFilters; ++i) {
                const auto shift = filter_width - i * mask_width;
                const auto f = (hash_v & (mask << shift)) >> shift;
                assert(0 <= f && f <= mask);
                filter_mask |= std::uint64_t{1} << f;
            }

            return filter_mask;
        }


        using BloomFilter = std::atomic<std::uint64_t>;
        BloomFilter m_bloom_filter{};
        static_assert(BloomFilter::is_always_lock_free);

        BucketBase m_bucket;
    };
}

#endif //EXERCISE5_BLOOM_FILTER_H
