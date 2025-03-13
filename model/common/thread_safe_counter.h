// project/common/thread_safe_counter.h
#ifndef THREAD_SAFE_COUNTER_H // Include guard to prevent multiple inclusions
#define THREAD_SAFE_COUNTER_H

#include <mutex>
#include <atomic>
#include <string>

class ThreadSafeCounter {
private:
    mutable std::mutex mtx;
    int counter = 0;

public:
    int increment();
    int get_value() const;
    void print_value(const std::string& message) const;
};

#endif