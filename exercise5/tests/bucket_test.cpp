
#include <gtest/gtest.h>

#include "bucket.h"
#include "bloom_filter.h"


template<epcpp::concepts::Bucket B>
class BucketTest : public ::testing::Test {
};

using BucketTypes = ::testing::Types<
        epcpp::ListBucket<int, int>,
        epcpp::BloomFilterAdapter<epcpp::ListBucket<int, int>>,
        epcpp::BloomFilterAdapter<epcpp::ListBucket<int, int>, 10>
>;
TYPED_TEST_SUITE(BucketTest, BucketTypes);

TYPED_TEST(BucketTest, InsertRemove) {
    TypeParam bucket;

    int num_elements = 5;

    auto hash = [](uint64_t x) {
        x ^= x >> 33U;
        x *= UINT64_C(0xff51afd7ed558ccd);
        x ^= x >> 33U;
        x *= UINT64_C(0xc4ceb9fe1a85ec53);
        x ^= x >> 33U;
        return static_cast<size_t>(x);
    };

    for (int i = 0; i < num_elements; ++i) {
        bool new_insert = bucket.insert(std::pair{i, i}, hash(i));
        ASSERT_TRUE(new_insert);
    }
    for (int i = 0; i < num_elements; ++i) {
        bool new_insert = bucket.insert(std::pair{i, i + 2}, hash(i));
        ASSERT_FALSE(new_insert);
    }
    for (int i = num_elements; i < 2 * num_elements; ++i) {
        bool removed = bucket.remove(i, hash(i));
        ASSERT_FALSE(removed);
    }
    for (int i = 0; i < num_elements; ++i) {
        bool removed = bucket.remove(i, hash(i));
        ASSERT_TRUE(removed);
    }
}
