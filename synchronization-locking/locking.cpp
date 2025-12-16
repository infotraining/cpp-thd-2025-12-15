#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <map>
#include <optional>
#include <shared_mutex>

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

struct Dns
{
    std::map<std::string, std::string> ip_lookup_;
    std::shared_mutex mtx_ip_lookup_;
public:
    explicit Dns(std::map<std::string, std::string> ip_lookup) : ip_lookup_{std::move(ip_lookup)}
    {
    }

    std::optional<std::string> get(const std::string& name)
    {
        {
            std::shared_lock lk{mtx_ip_lookup_}; // mtx_ip_lookup_.lock_shared() - reader lock
            std::this_thread::sleep_for(1s);
            if (auto pos = ip_lookup_.find(name); pos != ip_lookup_.end())
            {
                return pos->first;
            }
        }

        return std::nullopt;
    }

    void update(const std::string& name, const std::string& ip)
    {
        std::scoped_lock lk{mtx_ip_lookup_}; // writer lock - exclusive lock
        ip_lookup_[name] = ip; // modify ip_lookup
        std::cout << "Update done!" << std::endl;
    }
};

void using_dns()
{
    Dns dns({{"google.com", "1.1.1.1"}, {"cloudflare", "4.4.4.4"}});

    std::jthread thd_writer{[&dns] { 
        std::this_thread::sleep_for(200ms);
        dns.update("google.com", "2.2.2.2"); 
    }};

    {
        std::vector<std::jthread> threads;
        for (int i = 0; i < 10; ++i)
        {
            threads.emplace_back([&dns] { std::cout << "ip: " << dns.get("google.com").value_or("not-found") << std::endl; });
        }
    }
}

void counter_demo()
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

int main()
{
    using_dns();
}