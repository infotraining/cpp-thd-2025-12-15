#include "thread_safe_queue.hpp"

#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals;

using Task = std::function<void()>;

namespace PoisoiningPill
{
    static const Task stop_task;

    class ThreadPool
    {
    public:
        ThreadPool(size_t size)
        {
            threads_.reserve(size);
            for (auto i{0}; i < size; i++)
            {
                threads_.push_back(std::jthread{[this]() { run(); }});
            }
        }

        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;
        ThreadPool(ThreadPool&&) = delete;
        ThreadPool& operator=(ThreadPool&&) = delete;

        ~ThreadPool()
        {
            // sending poisoning pills to all threads
            for (size_t i = 0; i < threads_.size(); ++i)
                tasks_.push(stop_task);

            // for(auto& thd : threads_)
            //     thd.join();
        }

        void submit(Task task)
        {
            assert(task);
            tasks_.push(task);
        }

    private:
        ThreadSafeQueue<Task> tasks_;
        std::vector<std::jthread> threads_; // order of declaration for jthread matters

        void run()
        {
            while (true)
            {
                Task task;
                tasks_.pop(task); // waiting for task

                if (!task) // poisoning pill
                    break;

                task(); // executing task in working thread
            }
        }
    };
} // namespace PoisoiningPill

class ThreadPool
{
public:
    ThreadPool(size_t size)
    {
        threads_.reserve(size);
        for (auto i{0}; i < size; i++)
        {
            threads_.push_back(std::jthread{[this]() { run(); }});
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    ~ThreadPool()
    {
        for (size_t i = 0; i < threads_.size(); ++i)
            tasks_.push([this] { is_done_ = true; });
    }

    void submit(Task task)
    {
        tasks_.push(task);
    }

private:
    ThreadSafeQueue<Task> tasks_;
    std::vector<std::jthread> threads_;
    std::atomic<bool> is_done_{false};

    void run()
    {
        while (!is_done_)
        {
            Task task;
            tasks_.pop(task); // waiting for task

            task(); // executing task in working thread
        }
    }
};

void background_work(size_t id, const std::string& text, std::chrono::milliseconds delay)
{
    std::cout << "bw#" << id << " has started..." << std::endl;

    for (const auto& c : text)
    {
        std::cout << "bw#" << id << ": " << c << " in THD#" << std::this_thread::get_id() << std::endl;

        std::this_thread::sleep_for(delay);
    }

    std::cout << "bw#" << id << " is finished..." << std::endl;
}

int main()
{
    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    {
        ThreadPool thd_pool(10);

        thd_pool.submit([text] { background_work(1, text, 250ms); });

        for (int i = 2; i <= 20; ++i)
        {
            thd_pool.submit([text, i] { background_work(i, text, std::chrono::milliseconds{i * 20}); });
        }
    }

    std::cout << "Main thread ends..." << std::endl;
}
