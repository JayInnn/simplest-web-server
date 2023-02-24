#ifndef _LOG_H
#define _LOG_H

#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <cassert>
#include <cstring>       // memset
#include <cstdarg>       // vastart va_end
#include <sys/stat.h>    // mkdir
#include <sys/time.h>    // time

#include "blocking_queue.h"

/**
 * @brief 简单日志功能实现，支持功能：
 *  1. 支持同步、异步多线程写入
 *  2. 支持按行数分割日志
 *  3. 日志参数支持热更新
 */

enum LOG_LEVEL {
    DEBUG, 
    INFO, 
    WARN, 
    ERROR
};

class Log {

public:
    static Log *get_instance();

    void init(const char* path, int max_size, LOG_LEVEL level, 
            bool async_flag, int queue_capacity, int thread_num);

    void write(LOG_LEVEL level, const char *format, ...);

private:
    Log();
    ~Log();
    void async_flush();
    const char* get_level_title(LOG_LEVEL level);

private:
    static const int LOG_NAME_LEN = 256;
    static const int LOG_BUFFER_LEN = 20480;


    const char* dir_name;      // 路径名
    int max_row_per_file;      // 每个日志文件的最大行数
    int cur_row;               // 当前行数
    int cur_day_file_total;    // 当天日志文件个数
    int cur_today;             // 当前的日期

    LOG_LEVEL log_level;
    char* line_buf;

    bool async_flag;           // 异步写入相关属性
    std::unique_ptr<blocking_queue<std::string>> log_block_queue;
    std::vector<std::thread> log_write_thread_vec;

    FILE *_fp;                 // 日志文件fd
    mutable std::mutex _mutex;
};


//TODO: 输出日志新增行号
#define LOG_DEBUG(format, ...) Log::get_instance()->write(LOG_LEVEL::DEBUG, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...) Log::get_instance()->write(LOG_LEVEL::INFO, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...) Log::get_instance()->write(LOG_LEVEL::WARN, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) Log::get_instance()->write(LOG_LEVEL::ERROR, format, ##__VA_ARGS__)


#endif