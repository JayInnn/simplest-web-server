#ifndef _BLOCKING_QUEUE_H
#define _BLOCKING_QUEUE_H

#include <mutex>
#include <queue>
#include <atomic>
#include <condition_variable>

template<typename T>
class blocking_queue {

public:
    blocking_queue(): capacity(1024), destroy_flag(false) {}

    explicit blocking_queue(int cap): capacity(cap), destroy_flag(false) {}

    blocking_queue(const blocking_queue&) = delete;
    blocking_queue& operator=(const blocking_queue&) = delete;

    ~blocking_queue(){}

    /**
     * @brief 生产方
     * 1. 队列使用中，继续生产
     * 2. 队列不再使用，直接忽略
     * 
     * @param value 
     */
    void push(T& value) {
        if(destroy_flag.load()) {
            return;
        }

        std::unique_lock<std::mutex> locker(_mutex);
        cond_producer.wait(locker, [this]{return _queue.size() < capacity;});

        _queue.push(std::move(value));
        cond_consumer.notify_one();
    }

    /**
     * @brief 需要考虑四种情况
     * 1. 队列使用中，队列为空时，consumer等待
     * 2. 队列使用中，队列非空时，consumer正常消费
     * 3. 队列不再使用，队列为空时，consumer线程要正常结束，回收资源
     * 4. 队列不再使用，队列非空时，consumer线程需要继续队列中的数据进行处理
     * 
     * @param value 
     * @return true 
     * @return false 
     */
    bool pop(T& value) {
        std::unique_lock<std::mutex> locker(_mutex);

        cond_consumer.wait(locker, [this]{return (!_queue.empty()) || destroy_flag.load();});
        if(_queue.empty()) {
            return false;
        }
        value = _queue.front();
        _queue.pop();
        if(!destroy_flag.load()) {
            cond_producer.notify_one();
        }
        return true;
    }

    /**
     * @brief 不再允许写入数据
     */
    void stop() {
        destroy_flag.store(true);
        // 停止时可能存在线程wait，需要唤醒
        cond_consumer.notify_all();
        cond_producer.notify_all();
    }

    /**
     * @brief 该队列是否可用
     */
    bool is_finish() {
        std::lock_guard<std::mutex> locker(_mutex);
        return _queue.empty() && destroy_flag.load();
    }

private:
    int capacity;

    mutable std::mutex _mutex;
    
    std::atomic<bool> destroy_flag;

    std::queue<T> _queue;

    std::condition_variable cond_consumer;

    std::condition_variable cond_producer;
};


#endif