#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <mutex>

using namespace std::literals;

template <typename T>
struct SynchronizedValue
{
    T value;
    std::mutex mtx_value;

    
    [[nodiscard("Must be assigned to local variable to start critical section")]] std::lock_guard<std::mutex> lock()
    {
        return std::lock_guard{mtx_value};
    }
};

void may_throw()
{
    //throw std::runtime_error("Error#13");
}

void run(int& value, std::mutex& mtx_counter)
{
    for(int i = 0; i < 10'000'000; ++i)
    {
        std::lock_guard lk{mtx_counter}; // begin critical section
        ++value;
        may_throw();
    } // end critical section
}

void run(SynchronizedValue<int>& counter)
{
    for(int i = 0; i < 10'000'000; ++i)
    {
        auto lk = counter.lock(); // begin critical section
        ++counter.value;
        may_throw();
    } // end critical section
}

void run(std::atomic<int>& value)
{
    for(int i = 0; i < 10'000'000; ++i)
    {
        ++value;
    } // end critical section
}

int main()
{
    std::cout << "Main thread starts..." << std::endl;

    int counter = 0;
    std::mutex mtx_counter;
    
    SynchronizedValue<int> sync_counter{};

    std::atomic<int> atomic_counter{};
    
    {
        std::jthread thd_1{[&counter, &mtx_counter] { run(counter, mtx_counter); }};
        std::jthread thd_2{[&counter, &mtx_counter] { run(counter, mtx_counter); }};
        std::jthread thd_3{[&sync_counter] { run(sync_counter); }};
        std::jthread thd_4{[&sync_counter] { run(sync_counter); }};
        std::jthread thd_5{[&atomic_counter] { run(atomic_counter); }};
        std::jthread thd_6{[&atomic_counter] { run(atomic_counter); }};
    }

    
    std::cout << "counter: " << counter << "\n";
    std::cout << "sync_counter: " << sync_counter.value << "\n";
    std::cout << "atomic_counter: " << atomic_counter << "\n";

    std::cout << "Main thread ends..." << std::endl;
}
