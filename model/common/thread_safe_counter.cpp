// project/common/thread_safe_counter.cpp
#include "thread_safe_counter.h"
#include <iostream> // For print_value


int ThreadSafeCounter::increment() {
    std::lock_guard<std::mutex> lock(mtx);
    return ++counter;
}

int ThreadSafeCounter::get_value() const {
    std::lock_guard<std::mutex> lock(mtx);
    return counter;
}

void ThreadSafeCounter::print_value(const std::string& message) const {
    std::lock_guard<std::mutex> lock(mtx);
    std::cout << message << counter << std::endl;
}