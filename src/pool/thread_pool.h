#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H

#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <atomic>
#include <stdexcept>
#include <functional>

template<typename T>
class threadsafe_queue {

private:
    mutable std::mutex _mutex;
    std::queue<T> _queue;

public:
    void push(T data) {
        std::lock_guard<std::mutex> lg(_mutex);
        _queue.push(data);
    }

    bool pop(T& value) {
        std::lock_guard<std::mutex> lg(_mutex);
        if(_queue.empty()) {
            return false;
        }
        value = std::move(_queue.front());
        _queue.pop();
        return true;
    }
};


class thread_pool {

private:
    class join_threads {
    private:
        std::vector<std::thread> threads_;
    
    public:
        explicit join_threads(std::vector<std::thread>& th): threads_(th) {}
        ~join_threads() {
            for(unsigned i = 0; i < threads_.size(); ++i) {
                if(threads_[i].joinable()) {
                    threads_[i].join();
                }
            }
        }
    };

    std::atomic_bool done;
    threadsafe_queue<std::function<void()>> work_queue;
    std::vector<std::thread> threads;
    join_threads joins;

    void worker_thread() {
        while(!done) {
            std::function<void()> task;
            if(work_queue.pop(task)) {
                task();
            } else {
                std::this_thread::yield;
            }
        }
    }

public:
    thread_pool(unsigned capacity): done(false), joins(threads) {
        unsigned count = std::thread::hardware_concurrency();
        if(capacity > count) {
            throw std::out_of_range("thread pool init capacity over hardware count");
        }
        try {
            for(unsigned i = 0; i < capacity; ++i) {
                threads.push_back(std::thread(&thread_pool::worker_thread, this));
            }
        } catch(...) {
            done = true;
            throw;
        }
    }

    ~thread_pool() {
        done = true;
    }

    template<typename FunctionType>
    void submit(FunctionType f) {
        work_queue.push(std::function<void>(f));
    }
    
};


#endif