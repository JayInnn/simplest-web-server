#include "gtest/gtest.h"
#include "thread_pool.h"

TEST(test_threadsafe_queue, push) {
    threadsafe_queue<int> tp;
    tp.push(1);
    int result = 0;
    tp.pop(result);
    EXPECT_EQ(result, 1);
}