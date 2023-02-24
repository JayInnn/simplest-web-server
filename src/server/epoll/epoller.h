#ifndef _EPOLLER_H
#define _EPOLLER_H

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>

#include <vector>
#include <cassert>

class epoller {
public:
    explicit epoller(int max_event_size): _epoll_fd(epoll_create(512)), _events(max_event_size) {
        assert(_epoll_fd >= 0 && _events.size() > 0);
    }

    ~epoller() {
        if(_epoll_fd >= 0) {
            close(_epoll_fd);
        }
    }

    epoller(const epoller&) = delete;
    epoller& operator=(const epoller&) = delete;


public:
    void add_fd(int fd, uint32_t events) {
        struct epoll_event event;
        event.data.fd = fd;
        event.events = events;

        int epl_ctl = epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &event);
        assert(epl_ctl >= 0);

        int op = fcntl(fd, F_GETFL);
        op = op | O_NONBLOCK;
        fcntl(fd, F_SETFL, op);
    }

    void mod_fd(int fd, uint32_t events) {
        assert(fd >= 0);
        struct epoll_event event;
        event.data.fd = fd;
        event.events = events;
        epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, fd, &event);
    }

    void del_fd(int fd) {
        assert(fd >= 0);
        epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    }

    int wait(int timeout = -1) {
        return epoll_wait(_epoll_fd, &_events[0], static_cast<int>(_events.size()), timeout);
    }

    int get_event_fd(size_t i) const {
        assert(i < _events.size() && i >= 0);
        return _events[i].data.fd;
    }

    uint32_t get_event(size_t i) const {
        assert(i < _events.size() && i >= 0);
        return _events[i].events;
    }

private:
    int _epoll_fd;
    std::vector<struct epoll_event> _events;

};

#endif