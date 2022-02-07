#include <gtest/gtest.h>

#include <bitset>

#include "marked_ptr.h"

TEST(MarkedPtr, Create) {
    epcpp::marked_ptr<int> ptr;

    ASSERT_FALSE(ptr.is_marked());

    ptr = new int;

    ASSERT_FALSE(ptr.is_marked());

    std::cerr << ptr.get() << std::endl;
    std::cerr << std::bitset<64>((std::size_t)ptr.get()) << std::endl;

    auto ptr2 = ptr.as_marked();

    ASSERT_TRUE(ptr2.is_marked());

    std::atomic<epcpp::marked_ptr<int>> atomic_ptr;
    ASSERT_TRUE(std::atomic<epcpp::marked_ptr<int>>::is_always_lock_free);

}
