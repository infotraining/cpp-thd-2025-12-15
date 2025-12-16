#include <cassert>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <syncstream>
#include <thread>
#include <vector>
#include <mutex>

using namespace std::literals;

auto sync_cout()
{
    return std::osyncstream(std::cout);
}

static_assert(std::atomic<bool>::is_always_lock_free);

namespace BusyWaits
{
    class Data
    {
        std::vector<int> data_;
        std::atomic<bool> is_data_ready_{};

    public:
        void read()
        {
            sync_cout() << "Start reading..." << std::endl;
            data_.resize(100);

            std::random_device rnd;
            std::this_thread::sleep_for(2s);
            std::generate(begin(data_), end(data_), [&rnd] { return rnd() % 1000; });
            sync_cout() << "End reading..." << std::endl;
            // is_data_ready_ = true;
            is_data_ready_.store(true, std::memory_order_release);
        }

        void process(int id)
        {
            sync_cout() << "Start processing..." << std::endl;
            // while (!is_data_ready_) {}
            while (!is_data_ready_.load(std::memory_order_acquire)) { }
            long sum = std::accumulate(begin(data_), end(data_), 0L);
            sync_cout() << "Id: " << id << "; Sum: " << sum << std::endl;
        }
    };
} // namespace BusyWaits

inline namespace IdleWaits
{
    class Data
    {
        std::vector<int> data_;
        
        bool is_data_ready_{};
        std::mutex mtx_is_data_ready_;
        std::condition_variable cv_data_ready_;

    public:
        void read()
        {
            sync_cout() << "Start reading..." << std::endl;
            data_.resize(100);

            std::random_device rnd;
            std::this_thread::sleep_for(2s);
            std::generate(begin(data_), end(data_), [&rnd] { return rnd() % 1000; });
            sync_cout() << "End reading..." << std::endl;
            
            {
                std::scoped_lock lk{mtx_is_data_ready_};
                is_data_ready_ = true;
            }
            cv_data_ready_.notify_all();
        }

        void process(int id)
        {
            sync_cout() << "Start processing..." << std::endl;
            
            // std::unique_lock lk{mtx_is_data_ready_};
            // while (!is_data_ready_) 
            // {
            //     cv_data_ready_.wait(lk);
            // }

            std::unique_lock lk{mtx_is_data_ready_};
            cv_data_ready_.wait(lk, [this] { return is_data_ready_; });
            lk.unlock(); // end of critical section
            
            long sum = std::accumulate(begin(data_), end(data_), 0L);
            sync_cout() << "Id: " << id << "; Sum: " << sum << std::endl;
        }
    };
}

int main()
{
    {
        Data data;
        std::jthread thd_producer{[&data] { data.read(); }};

        std::jthread thd_consumer_1{[&data] { data.process(1); }};
        std::jthread thd_consumer_2{[&data] { data.process(2); }};
        std::jthread thd_consumer_3{[&data] { data.process(3); }};
    }

    sync_cout() << "END of main..." << std::endl;
}
