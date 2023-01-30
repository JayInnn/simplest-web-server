#include "gtest/gtest.h"
#include "log.h"
#include <future>


Log* get_log_instance() {
    return Log::get_instance();
}

TEST(test_log_get_instance, get_instance) {
    Log* instance = Log::get_instance();
    std::future<Log*> ft01 = std::async(get_log_instance);
    std::future<Log*> ft02 = std::async(get_log_instance);

    EXPECT_EQ(ft01.get(), instance);
    EXPECT_EQ(ft02.get(), instance);
}