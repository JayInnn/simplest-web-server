#include <thread_pool.h>
#include <iostream>

int main() {
    threadsafe_queue<int> sq;
    sq.push(1);
    std::cout << "success" << std::endl;
}