
#include <gtest/gtest.h>

#include "hash_map.h"

TEST(HashMapTest, Find) {
    epcpp::hash_map<int, int> map(10);

    int k = 100;

    for (int i = 0; i < k; ++i) {
        auto [h, inserted] = map.find_or_insert(i, 2 * i);
        ASSERT_TRUE(h);
        ASSERT_NE(h, map.end());
        ASSERT_TRUE(inserted);
        ASSERT_EQ(h->first, i);
        ASSERT_EQ(h->second, 2 * i);

        h.reset();
        ASSERT_FALSE(h);
        ASSERT_EQ(h, map.end());
    }

    for (int i = 0; i < k; ++i) {
        auto h = map.find(i);
        ASSERT_TRUE(h);
        ASSERT_NE(h, map.end());
        ASSERT_EQ(h->first, i);
        ASSERT_EQ(h->second, 2 * i);

        h.reset();
        ASSERT_FALSE(h);
        ASSERT_EQ(h, map.end());
    }

    for (int i = k; i < 2 * k; ++i) {
        auto h = map.find(i);
        ASSERT_FALSE(h);
        ASSERT_EQ(h, map.end());
    }

    for (int i = 0; i < k; ++i) {
        auto [h, inserted] = map.find_or_insert(i, 3 * i);
        h->second = 3 * i;
        ASSERT_TRUE(h);
        ASSERT_NE(h, map.end());
        ASSERT_FALSE(inserted);
        ASSERT_EQ(h->first, i);
        ASSERT_EQ(h->second, 3 * i);

        h.reset();
        ASSERT_FALSE(h);
        ASSERT_EQ(h, map.end());
    }

    for (int i = 0; i < k; ++i) {
        auto h = map.find(i);
        ASSERT_TRUE(h);
        ASSERT_NE(h, map.end());
        ASSERT_EQ(h->first, i);
        ASSERT_EQ(h->second, 3 * i);

        h.reset();
        ASSERT_FALSE(h);
        ASSERT_EQ(h, map.end());
    }

    for (int i = k; i < 2 * k; ++i) {
        auto h = map.find(i);
        ASSERT_FALSE(h);
        ASSERT_EQ(h, map.end());
    }

    for (int i = k; i < 2 * k; ++i) {
        auto erased = map.erase(i);
        ASSERT_FALSE(erased);
    }

    for (int i = 0; i < k; ++i) {
        auto erased = map.erase(i);
        ASSERT_TRUE(erased) << i;
    }
}