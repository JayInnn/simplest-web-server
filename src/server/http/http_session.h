#ifndef _HTTP_SESSION_H
#define _HTTP_SESSION_H

#include <memory>

#include "http_request.h"
#include "http_response.h"
#include "../epoll/epoller.h"

constexpr int READ_BUFFER_SIZE  = 20480;
constexpr int WRITE_BUFFER_SIZE = 1024;

class http_session {
public: 
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT,
        CHECK_STATE_FINISH,
        CHECK_STATE_ERROR
    };

public:
    http_session(int fd_, uint32_t event, std::shared_ptr<epoller>& epl);
    http_session(const http_session&) = delete;
    http_session& operator=(const http_session&) = delete;
    ~http_session() = default;

    bool read_buf();
    void process();
    bool write_buf();

private:
    void process_read_buf();
    bool parse_line();
    bool parse_request_line(std::string&);
    bool parse_headers(std::string&);
    bool parse_content(std::string&);

private:
    int fd;
    uint32_t conn_event;
    std::shared_ptr<epoller> epler_;

    // read buffer
    char m_read_buf[READ_BUFFER_SIZE];
    int  m_read_idx;
    int  m_checked_idx;
    int  m_start_line;

    // write buffer
    int iov_cnt;
    struct iovec iov_vec[2];
    int bytes_to_send;
    int bytes_have_send;

    CHECK_STATE   m_check_state;
    http_request  request;
    http_response response;

};

#endif