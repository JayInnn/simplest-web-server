#include "gtest/gtest.h"
#include "blocking_queue.h"

TEST(test_blocking_queue, stop) {
    blocking_queue<std::string> bq;
    bq.stop();
    EXPECT_TRUE(bq.is_finish());
}