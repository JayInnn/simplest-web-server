#include "gtest/gtest.h"
#include "log.h"
#include <future>
#include <unistd.h>

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

TEST(test_log_sync_init, init) {
    Log* instance = Log::get_instance();
    std::string path("./");
    instance->init(path.c_str(), 10, LOG_LEVEL::INFO, false, 10, 2);

    std::time_t timer = std::time(nullptr);
    std::tm *sysTime = std::localtime(&timer);
    char file_name[256] = {0};
    snprintf(file_name, 256 - 1, "%s/application_%04d_%02d_%02d.log_%02d", 
            path.c_str(), sysTime->tm_year + 1900, sysTime->tm_mon + 1, sysTime->tm_mday, 0);

    bool is_exist = false;
    if(access(file_name, F_OK) == 0) {
        is_exist = true;
    }
    EXPECT_TRUE(is_exist);
    
    int res = unlink(file_name);
    ASSERT_EQ(res, 0);
}

TEST(test_log_async_init, init) {
    Log* instance = Log::get_instance();
    std::string path("./");
    instance->init(path.c_str(), 10, LOG_LEVEL::INFO, true, 10, 2);

    std::time_t timer = std::time(nullptr);
    std::tm *sysTime = std::localtime(&timer);
    char file_name[256] = {0};
    snprintf(file_name, 256 - 1, "%s/application_%04d_%02d_%02d.log_%02d", 
            path.c_str(), sysTime->tm_year + 1900, sysTime->tm_mon + 1, sysTime->tm_mday, 0);

    // file exist
    bool is_exist = false;
    if(access(file_name, F_OK) == 0) {
        is_exist = true;
    }
    EXPECT_TRUE(is_exist);

    // expect thread num = 2
    __pid_t cur_pid = getpid();
    char buf[128] = {0};
    snprintf(buf, sizeof(buf), "cat /proc/%d/status | grep 'Threads' | grep -Eo '[0-9]+$'", cur_pid);    
	FILE* fp = popen(buf, "r");

    char ret[8] = {0};
    int nread = fread(ret, 1, sizeof(ret), fp);
    pclose(fp);
    std::string tmp = ret;
    ASSERT_EQ(tmp, "3\n");

    
    int res = unlink(file_name);
    ASSERT_EQ(res, 0);
}

