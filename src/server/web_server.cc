#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "web_server.h"

web_server::web_server(EPOLL_MODE mode, int idle_time_ms, socket_options& opt):
    epoll_mode(mode), max_rdwd_idle_time(idle_time_ms), options(opt),
    threadpool_(new thread_pool(8)), epler_(std::make_shared<epoller>(1024)), timer_(new heap_timer()) {

    init_epoll_mode();
    char* path = getcwd(nullptr, 256);
    Log::get_instance()->init(path, 20480, LOG_LEVEL::INFO, true, 512, 2);
    LOG_INFO("========== log init finish ==========");

    init_socket();
    LOG_INFO("========== server init finish ==========");
    LOG_INFO("port: %d, opt_linger: %d, opt_reuseaddr: %d", options.port, options.opt_linger, options.opt_reuseaddr);
}

web_server::~web_server() {
    server_ready = false;
    if(sock_fd >= 0) {
        close(sock_fd);
    }
}

void web_server::start() {
    int time_ms = -1;
    server_ready = true;
    while(server_ready) {
        if(time_ms > 0) {
            time_ms = timer_->get_next_tick();
        }
        int epl_num = epler_->wait(time_ms);
        for(int i = 0; i < epl_num; ++i) {
            int fd = epler_->get_event_fd(i);
            uint32_t event = epler_->get_event(i);
            if(fd == sock_fd) {
                deal_listen(fd);
            } else if(event & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                deal_close(fd);
            } else if(event & EPOLLIN) {
                deal_read(fd);
            } else if(event & EPOLLOUT) {
                deal_write(fd);
            } else {
                LOG_ERROR("unexpected event!!!");
            }
        }
    }
}

void web_server::check_user_exist(int fd) {
    if(users_.count(fd) == 0) {
        LOG_ERROR("check user exist, fd(%d) has no users.", fd);
        users_.erase(fd);
    }
}

void web_server::init_socket() {
    assert(options.port < 65535 && options.port > 1024);
    int sock_fd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock_fd >= 0);

    // set socket options
    if(options.opt_linger) {
        /* 优雅关闭: 直到所剩数据发送完毕或超时 */
        struct linger lg;
        bzero(&lg, sizeof(lg));
        lg.l_linger = 1;
        lg.l_onoff = 1;
        int ret = setsockopt(sock_fd, SOL_SOCKET, SO_LINGER, &lg, sizeof(lg));
        assert(ret == 0);
    }

    if(options.opt_reuseaddr) {
        /* 端口复用 */
        int val = 1;
        int ret = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (const void*)&val, sizeof(int));
        assert(ret == 0);
    }

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(options.port);

    int ret = 0;
    ret = bind(sock_fd, (struct sockaddr *)&address, sizeof(address));
    assert(ret >= 0);

    ret = listen(sock_fd, SOMAXCONN);   // SOMAXCONN: 监听队列最多容纳数量
    assert(ret >= 0);

    /* epoll多路复用 */
    epler_->add_fd(sock_fd, listen_event | EPOLLIN);
}

void web_server::deal_listen(int fd) {
    struct sockaddr_in client_address; 
    socklen_t client_addrlength = sizeof(client_address);
    do {
        int conn_fd = accept(fd,  (struct sockaddr *)(&client_address), &client_addrlength);
        if (conn_fd < 0) {
            return;
        }
        // TODO: server busy
        std::shared_ptr<http_session> session = std::make_shared<http_session>(conn_fd, conn_event, epler_);
        users_.insert(std::make_pair(conn_fd, session));
        if(max_rdwd_idle_time > 0) {
            timer_->add(conn_fd, max_rdwd_idle_time, std::bind(&web_server::deal_close, this, conn_fd));
        }
        epler_->add_fd(conn_fd, true);
    } while(listen_event & EPOLLET);
}

void web_server::deal_read(int fd) {
    check_user_exist(fd);
    std::shared_ptr<http_session> session = users_[fd];
    if(session->read_buf()) {
        threadpool_->submit(std::bind(&http_session::process, session.get()));
        if(max_rdwd_idle_time > 0) { 
            timer_->update(fd, max_rdwd_idle_time);
        }
    } else {
        timer_->del(fd, true);
    }
}

void web_server::deal_write(int fd) {
    check_user_exist(fd);
    std::shared_ptr<http_session> session = users_[fd];
    if(session->write_buf()) {
        if(max_rdwd_idle_time > 0) { 
            timer_->update(fd, max_rdwd_idle_time);
        }
    } else {
        timer_->del(fd, true);
    }
}

void web_server::deal_close(int fd) {
    if(fd < 0) {
        LOG_ERROR("deal close, fd is less than zero, fd: %d", fd);
        return;
    }
    epler_->del_fd(fd);
    //TODO: 是否考虑线程安全问题
    users_.erase(fd);
    LOG_INFO("deal close, fd(%d) is closed", fd);
}

void web_server::init_epoll_mode() {
    listen_event = EPOLLRDHUP;
    // 单线程处理conn fd
    conn_event = EPOLLONESHOT | EPOLLRDHUP;
    switch(epoll_mode) {
        case LISTEN_CONNECTION_ET:
            listen_event |= EPOLLET;
            conn_event   |= EPOLLET;
            break;
        case LISTEN_LT_CONNECTION_ET:
            conn_event   |= EPOLLET;
            break;
        case LISTEN_ET_CONNECTION_LT:
            listen_event |= EPOLLET;
            break;
        default:
            break;
    }
}