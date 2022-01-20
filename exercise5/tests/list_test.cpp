#include <list>

#include <gtest/gtest.h>


template <class List>
class ListTest : public ::testing::Test {};

using ListTypes = ::testing::Types<std::list<int>>;
TYPED_TEST_SUITE(ListTest, ListTypes);

TYPED_TEST(ListTest, PushPop) {
    TypeParam list;
    ASSERT_EQ(list.size(), 0);
    for (int i = 1; i <= 5; ++i) {
        list.push_back(i);
        ASSERT_EQ(list.size(), i);
    }
    for (int i = 5; i >= 1; --i) {
        ASSERT_EQ(list.size(), i);
        list.pop_back();
    }
    ASSERT_EQ(list.size(), 0);
}


