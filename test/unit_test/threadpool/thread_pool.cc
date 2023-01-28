#include "gtest/gtest.h"
#include "thread_pool.h"

TEST(Test_Threadsafe_Queue, push) {
    threadsafe_queue<int> tp;
    tp.push(1);
    EXPECT_EQ(tp.pop(), 1);
}