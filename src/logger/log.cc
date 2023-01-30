#include "log.h"


Log::Log() {
    cur_row = 0;
    cur_day_file_total = 0;
    log_level = LOG_LEVEL::INFO;

    async_flag = false;

    line_buf = nullptr;
    _fp = nullptr;
}

Log::~Log() {
    if(async_flag || (!log_write_thread_vec.empty())) {
        log_block_queue->stop();
        for(unsigned i = 0; i < log_write_thread_vec.size(); ++i) {
            if(log_write_thread_vec[i].joinable()) {
                log_write_thread_vec[i].join();
            }
        }
    }

    if(_fp != nullptr) {
        std::lock_guard<std::mutex> locker(_mutex);
        fflush(_fp);
        fclose(_fp);
    }

    if(line_buf != nullptr) {
        delete []line_buf;
    }
}


Log *Log::get_instance() {
    static Log instance;
    return &instance;
}

void Log::init(const char* path, int max_row, LOG_LEVEL level, 
    bool async_, int queue_capacity, int thread_num) {
    
    max_row_per_file = max_row;
    cur_day_file_total = 0;
    log_level = level;
    dir_name = path;

    line_buf = new char[LOG_BUFFER_LEN];
    memset(line_buf, '\0', LOG_BUFFER_LEN);

    // get fd by log name
    std::time_t timer = std::time(nullptr);
    std::tm *sysTime = std::localtime(&timer);
    char file_name[LOG_NAME_LEN] = {0};
    snprintf(file_name, LOG_NAME_LEN - 1, "%s/application_%04d_%02d_%02d.log_%02d", 
            path, sysTime->tm_year + 1900, sysTime->tm_mon + 1, sysTime->tm_mday, cur_day_file_total);
    cur_today = sysTime->tm_mday;
    {
        std::lock_guard<std::mutex> locker(_mutex);
        if(_fp) { 
            fflush(_fp);
            fclose(_fp);
        }

        _fp = fopen(file_name, "a");
        if(_fp == nullptr) {
            mkdir(path, 0777);
            _fp = fopen(file_name, "a");
        } 
        assert(_fp != nullptr);
    }

    // async config
    if(async_) {
        async_flag = true;
        std::unique_ptr<blocking_queue<std::string>> temp_queue(new blocking_queue<std::string>(queue_capacity));
        log_block_queue = std::move(temp_queue);
        for(unsigned i = 0; i < thread_num; ++i) {
            log_write_thread_vec.push_back(std::thread(&Log::async_flush, this));
        }
    } else {
        async_flag = false;
    }
}


void Log::write(LOG_LEVEL level, const char *format, ...) {
    if(level < log_level) {
        return;
    }


    // 按照日期分片
    // std::time_t timer = std::time(nullptr);
    // std::tm *sysTime = std::localtime(&timer);
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    std::time_t timer = now.tv_sec;
    std::tm *sysTime = localtime(&timer);
    {
        std::lock_guard<std::mutex> locker(_mutex);
        if(cur_today != sysTime->tm_mday) {
            cur_today = sysTime->tm_mday;
            cur_row = 0;
            cur_day_file_total = 0;
            char file_name[LOG_NAME_LEN] = {0};
            snprintf(file_name, LOG_NAME_LEN - 1, "%s/application_%04d_%02d_%02d.log_%02d", 
                dir_name, sysTime->tm_year + 1900, sysTime->tm_mon + 1, sysTime->tm_mday, cur_day_file_total);

            fflush(_fp);
            fclose(_fp);
            _fp = fopen(file_name, "a");
        }
    }

    // 按照行数分片
    {
        std::lock_guard<std::mutex> locker(_mutex);
        if(cur_row >= max_row_per_file) {
            cur_row = 0;
            cur_day_file_total++;
            char file_name[LOG_NAME_LEN] = {0};
            snprintf(file_name, LOG_NAME_LEN - 1, "%s/application_%04d_%02d_%02d.log_%02d", 
                dir_name, sysTime->tm_year + 1900, sysTime->tm_mon + 1, sysTime->tm_mday, cur_day_file_total);

            fflush(_fp);
            fclose(_fp);
            _fp = fopen(file_name, "a");
        }
    }

    // 写入文件
    {
        std::lock_guard<std::mutex> locker(_mutex);
        std::string line_str;
        va_list valst;
        va_start(valst, format);

        const char* level_prefix = get_level_title(level);
        int n = snprintf(line_buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                     sysTime->tm_year + 1900, sysTime->tm_mon + 1, sysTime->tm_mday,
                     sysTime->tm_hour, sysTime->tm_min, sysTime->tm_sec, now.tv_usec, level_prefix);

        int m = vsnprintf(line_buf + n, LOG_BUFFER_LEN - 1, format, valst);
        line_buf[n + m] = '\n';
        line_buf[n + m + 1] = '\0';
        line_str = line_buf;

        if(async_flag) {
            log_block_queue->push(line_str);
        } else {
            fputs(line_str.c_str(), _fp);
            cur_row++;
        }
        va_end(valst);
    }

}


void Log::async_flush() {
    std::string str("");
    while(!log_block_queue->is_finish()) {
        if(log_block_queue->pop(str)) {
            std::lock_guard<std::mutex> locker(_mutex);
            fputs(str.c_str(), _fp);
            cur_row++;
        }
    }
}


const char *Log::get_level_title(LOG_LEVEL level) {
    std::string str;
    switch (level) {
        case LOG_LEVEL::DEBUG:
            str = "[DEBUG]:";
            break;
        case LOG_LEVEL::INFO: 
            str = "[INFO]:";
            break;
        case LOG_LEVEL::WARN:
            str = "[WARN]:";
            break;
        case LOG_LEVEL::ERROR:
            str = "[ERROR]:";
            break;
        default:str = "[INFO]:";
    }
    return str.c_str();
}