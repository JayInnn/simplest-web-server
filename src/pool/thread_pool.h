#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H

#include <mutex>
#include <queue>


template<typename T>
class threadsafe_queue {

private:
    mutable std::mutex _mutex;
    std::queue<T> _queue;

public:
    void push(T data) {

    }

    void pop(T& value) {

    }

};


class thread_pool {

};


#endif