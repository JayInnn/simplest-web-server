#ifndef _HTTP_RESPONSE_H
#define _HTTP_RESPONSE_H

#include <string>
#include <unordered_map>
#include <fcntl.h>       // open
#include <unistd.h>      // close
#include <sys/stat.h>    // stat
#include <sys/mman.h>    // mmap, munmap

#include "../../logger/log.h"

class http_response {

public:
    http_response() {}
    ~http_response() {
        if(m_file) {
            munmap(m_file, m_file_stat.st_size);
            m_file = nullptr;
        }
    }
    http_response(const http_response&) = delete;
    http_response& operator=(const http_response&) = delete;

    void set_error_info();
    void set_finish_info(std::string path, bool keepalive);
    std::string build_response_body();
    char*  get_m_file();
    size_t get_file_len() const;


private:
    std::string get_content_type();
    std::string error_content(std::string message);
    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    static const std::unordered_map<int, std::string> CODE_PATH;

private:
    int rsp_code = -1;
    bool rsp_keepalive;

    std::string rsp_path;
    std::string rsp_resource_path = "./resource/";

    char* m_file;
    struct stat m_file_stat;
};

#endif