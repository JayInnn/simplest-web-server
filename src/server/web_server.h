#ifndef _WEB_SERVER_H
#define _WEB_SERVER_H

#include <memory>
#include <unordered_map>

#include "../pool/thread_pool.h"
#include "../timer/heap_timer.h"
#include "epoll/epoller.h"
#include "http/http_session.h"


/**
 * @brief socket options
 */
class socket_options {
public:
    socket_options(int p, bool linger, bool reuseaddr): 
        port(p) ,opt_linger(linger), opt_reuseaddr(reuseaddr) {}

    ~socket_options() = default;

    int port;

    bool opt_linger;

    bool opt_reuseaddr;
};

/**
 * @brief web server实例
 * 
 */
class web_server {
public:
    enum EPOLL_MODE
    {
        LISTEN_CONNECTION_LT = 0,
        LISTEN_CONNECTION_ET,
        LISTEN_LT_CONNECTION_ET,
        LISTEN_ET_CONNECTION_LT
    };


public:
    web_server(EPOLL_MODE mode, int idle_time_ms, socket_options& opt);
    web_server(const web_server&) = delete;
    web_server& operator=(const web_server&) = delete;
    ~web_server();

    void start();

private:
    void init_socket();
    void deal_listen(int fd);
    void deal_write(int fd);
    void deal_read(int fd);
    void deal_close(int fd);
    void check_user_exist(int fd);
    void init_epoll_mode();

private:
    int sock_fd;
    bool server_ready;
    int max_rdwd_idle_time;
    socket_options options;

    EPOLL_MODE epoll_mode;
    uint32_t listen_event;
    uint32_t conn_event;

    std::unique_ptr<thread_pool> threadpool_;
    std::unique_ptr<heap_timer> timer_;
    std::shared_ptr<epoller> epler_;
    std::unordered_map<int, std::shared_ptr<http_session>> users_;
};

#endif