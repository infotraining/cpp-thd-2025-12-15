#include "thread_safe_queue.hpp"

#include <cassert>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals;

#if __cplusplus >= 202302L
using Task = std::move_only_function<void()>;
#else
using Task = std::function<void()>;
#endif


#if __cplusplus < 202302L
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
#endif

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

    template <typename TTask>
    auto submit(TTask&& task) -> std::future<decltype(task())>
    {
        using TResult = decltype(task());

#if __cplusplus >= 202302L
        std::packaged_task<TResult()> pt(std::forward<TTask>(task));
        std::future<TResult> f_result = pt.get_future();
        tasks_.push(std::move(pt));
#else
        // work-around for std::function as Task
        auto pt = std::make_shared<std::packaged_task<TResult()>>(std::forward<TTask>(task));
        std::future<TResult> f_result = pt->get_future();
        tasks_.push([pt] { (*pt)(); });
#endif

        return f_result;
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

int calculate_square(int x)
{
    std::cout << "Starting calculation for " << x << " in " << std::this_thread::get_id() << std::endl;

    std::random_device rd;
    std::uniform_int_distribution<> distr(100, 5000);

    std::this_thread::sleep_for(std::chrono::milliseconds(distr(rd)));

    if (x % 3 == 0)
        throw std::runtime_error("Error#3");

    return x * x;
}

int main()
{
    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    {
        ThreadPool thd_pool(10);

        thd_pool.submit([text] { background_work(1, text, 250ms); });

        std::vector<std::tuple<int, std::future<int>>> f_squares;

        for (int i = 1; i <= 20; ++i)
        {
            f_squares.emplace_back(i, thd_pool.submit([i] { return calculate_square(i); }));
        }

        for (auto& [i, fs] : f_squares)
        {
            try
            {
                int result = fs.get();
                std::cout << i << "*" << i << " = " << result << std::endl;
            }
            catch (const std::exception& e)
            {
                std::cout << "Caught exception for " << i << ": " << e.what() << std::endl;
            }
        }
    }

    std::cout << "Main thread ends..." << std::endl;
}
