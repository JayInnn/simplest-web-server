#ifndef _HEAP_TIMER_H
#define _HEAP_TIMER_H

#include <chrono>
#include <functional>
#include <vector>
#include <unordered_map>
#include <cassert> 
#include <iostream>

typedef std::function<void()> timeout_callback;
typedef std::chrono::high_resolution_clock high_clock;
typedef std::chrono::milliseconds chrono_ms;
typedef typename high_clock::time_point timestamp;


struct timer_node {
    int fd;
    timestamp expires;
    timeout_callback cb;
    bool operator<(const timer_node& t) {
        return expires < t.expires;
    }
};


/**
 * @brief 基于小根堆实现的定时器，关闭超时的非活动连接
 * 
 */
class heap_timer {

public:
    heap_timer() {
        heap_.reserve(64);
    }

    ~heap_timer() {}

    void add(int fd, int time_out, const timeout_callback& cb) {
        assert(fd >= 0);
        size_t i;
        if(ref_.count(fd) == 0) {
            /* 新节点：堆尾插入，调整堆 */
            i = heap_.size(); 
            ref_[fd] = i;
            heap_.push_back({fd, high_clock::now() + chrono_ms(time_out), cb});
            upward_adjustment(i);
        } else {
            /* 已有节点：调整堆 */
            i = ref_[fd];
            heap_[i].expires = high_clock::now() + chrono_ms(time_out);
            heap_[i].cb = cb;
            if(!downward_adjustment(i, heap_.size())) {
                upward_adjustment(i);
            }
        }
    }

    void del(int fd, bool callback) {
        if(heap_.empty() || ref_.count(fd) == 0) {
            return;
        }
        size_t index = ref_[fd];
        if(callback) {
            timer_node node = heap_[index];
            node.cb();
        }
        
        assert(!heap_.empty() && index >= 0 && index < heap_.size());
        /* 将要删除的结点换到队尾，然后调整堆 */
        size_t i = index;
        size_t n = heap_.size() - 1;
        assert(i <= n);
        if(i < n) {
            swap_node(i, n);
            if(!downward_adjustment(i, n)) {
                upward_adjustment(i);
            }
        }
        /* 队尾元素删除 */
        ref_.erase(heap_.back().fd);
        heap_.pop_back();
    }

    void update(int fd, int time_out) {
        /* 调整指定fd的结点 */
        assert(!heap_.empty() && ref_.count(fd) > 0);
        heap_[ref_[fd]].expires = high_clock::now() + chrono_ms(time_out);
        if(!downward_adjustment(ref_[fd], heap_.size())) {
            upward_adjustment(ref_[fd]);
        }
    }

    void tick() {
        /* 清除超时结点 */
        if(heap_.empty()) {
            return;
        }
        while(!heap_.empty()) {
            timer_node node = heap_.front();
            if(std::chrono::duration_cast<chrono_ms>(node.expires - high_clock::now()).count() > 0) { 
                break;
            }
            node.cb();
            del(node.fd, false);
        }
    }

    size_t get_next_tick() {
        tick();
        size_t res = -1;
        if(!heap_.empty()) {
            res = std::chrono::duration_cast<chrono_ms>(heap_.front().expires - high_clock::now()).count();
            if(res < 0) { res = 0; }
        }
        return res;
    }

    void clear() {
        ref_.clear();
        heap_.clear();
    }

    bool empty() {
        return heap_.empty();
    }

    /**
     * @brief 输出最小堆详情
     * vector:
     *     [0] fd = xxx    expire = xxx
     *     [1] fd = xxx    expire = xxx
     * 
     * ref:
     *     key = xxx    value = xxx
     *     key = xxx    value = xxx
     * 
     */
    void print() {
        if(heap_.empty()) {
            return;
        }

        auto now_ = high_clock::now();
        std::cout << "vector: " << std::endl;
        for(auto i = 0; i < heap_.size(); ++i) {
            std::cout << "\t[" << i << "] ";
            std::cout << "fd = " << heap_[i].fd;
            std::cout << "\texpire duration = " << std::chrono::duration_cast<chrono_ms>(heap_[i].expires - now_).count() << "\n";
        }
        std::cout << "\n";

        std::cout << "ref: " << std::endl;
        for(auto it = ref_.begin(); it != ref_.end(); ++it) {
            std::cout << "\tfirst = " << it->first << "\tsecond = " << it->second << std::endl;
        }
        std::cout << "\n";
    }

private:
    void upward_adjustment(size_t i) {
        assert(i >= 0 && i < heap_.size());
        if(i < 1) {
            return;
        }
        size_t j = (i - 1) / 2;
        while(j >= 0) {
            if(heap_[j] < heap_[i]) {
                break;
            }
            swap_node(i, j);
            i = j;
            j = (i - 1) / 2;
        }
    }

    bool downward_adjustment(size_t index, size_t n) {
        assert(index >= 0 && index < heap_.size());
        assert(n >= 0 && n <= heap_.size());
        size_t i = index;
        size_t j = i * 2 + 1;
        while(j < n) {
            if(j + 1 < n && heap_[j + 1] < heap_[j]) j++;
            if(heap_[i] < heap_[j]) break;
            swap_node(i, j);
            i = j;
            j = i * 2 + 1;
        }
        return i > index;
    }

    void swap_node(size_t i, size_t j) {
        assert(i >= 0 && i < heap_.size());
        assert(j >= 0 && j < heap_.size());
        std::swap(heap_[i], heap_[j]);
        ref_[heap_[i].fd] = i;
        ref_[heap_[j].fd] = j;
    }

private:
    std::vector<timer_node> heap_;
    std::unordered_map<int, size_t> ref_;
};

#endif