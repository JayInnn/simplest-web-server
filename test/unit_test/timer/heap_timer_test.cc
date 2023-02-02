#include "gtest/gtest.h"
#include "heap_timer.h"
#include <thread>

void callback(int fd) {
    std::cout << "fd: " << fd << " finish callback" << std::endl;
}

TEST(test_heap_timer, add) {
    heap_timer timer;
    timer.add(1, 10, std::bind(callback, 1));
    timer.add(2, 100, std::bind(callback, 1));
    timer.add(3, 200, std::bind(callback, 1));
    
    EXPECT_FALSE(timer.empty());
}

TEST(test_heap_timer, del) {
    heap_timer timer;
    timer.add(1, 10, std::bind(callback, 1));
    timer.add(2, 100, std::bind(callback, 1));
    timer.add(3, 200, std::bind(callback, 1));
    
    std::cout << "========= del before\n";
    timer.print();
    std::cout << "========= del fd = 3 start\n";
    timer.del(3, false);
    std::cout << "========= del finish\n";
    timer.print();
    EXPECT_FALSE(timer.empty());
}

TEST(test_heap_timer, update) {
    heap_timer timer;
    timer.add(1, 10, std::bind(callback, 1));
    timer.add(2, 100, std::bind(callback, 1));
    timer.add(3, 200, std::bind(callback, 1));
    
    std::cout << "========= update before\n";
    timer.print();
    std::cout << "========= update fd = 3 start\n";
    timer.update(3, 300);
    std::cout << "========= update finish\n";
    timer.print();
    EXPECT_FALSE(timer.empty());
}

TEST(test_heap_timer, tick) {
    heap_timer timer;
    timer.add(1, 10, std::bind(callback, 1));
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    
    timer.tick();
    EXPECT_TRUE(timer.empty());

    timer.add(1, 10, std::bind(callback, 1));
    timer.add(2, 100, std::bind(callback, 1));
    timer.add(3, 200, std::bind(callback, 1));
    std::this_thread::sleep_for(std::chrono::milliseconds(120));
    std::cout << "========= tick again\n";
    timer.tick();
    EXPECT_FALSE(timer.empty());

}

TEST(test_heap_timer, clear) {
    heap_timer timer;
    timer.add(1, 10, std::bind(callback, 1));
    EXPECT_FALSE(timer.empty());

    timer.clear();
    EXPECT_TRUE(timer.empty());
}

TEST(test_heap_timer, print) {
    heap_timer timer;
    timer.add(1, 10, std::bind(callback, 1));
    timer.add(2, 100, std::bind(callback, 1));
    timer.add(3, 200, std::bind(callback, 1));

    timer.print();
}