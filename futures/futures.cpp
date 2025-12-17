#include <cassert>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals;

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

void save_to_file(const std::string& filename)
{
    std::cout << "Saving to file: " << filename << std::endl;

    std::this_thread::sleep_for(3s);

    std::cout << "File saved: " << filename << std::endl;
}

void future_simple_demo()
{
    std::future<int> f_square_13 = std::async(std::launch::async, &calculate_square, 13); // std::launch::async creates new thread and starts calulation
    std::future<int> f_square_9 = std::async(std::launch::deferred, [] { return calculate_square(9); });
    std::future<void> f_save = std::async(std::launch::async, &save_to_file, "data.txt");

    while (f_save.wait_for(100ms) != std::future_status::ready)
    {
        std::cout << ".";
        std::cout.flush();
    }

    int square_13 = f_square_13.get(); // waiting for result
    std::cout << "13 * 13 = " << square_13 << std::endl;

    try
    {
        int square_9 = f_square_9.get();
        std::cout << "9 * 9 = " << square_9 << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cout << "Caught exception: " << e.what() << std::endl;
    }

    f_save.wait();
}

void future_grouped_tasks()
{
    std::vector<std::tuple<int, std::future<int>>> f_squares;

    for (int i = 1; i <= 20; ++i)
    {
        f_squares.emplace_back(i, std::async(std::launch::async, &calculate_square, i));
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

void bug_in_standard()
{
    // BUG!!!
    // std::async(std::launch::async, &save_to_file, "data1.txt");
    // std::async(std::launch::async, &save_to_file, "data2.txt");
    // std::async(std::launch::async, &save_to_file, "data3.txt");
    // std::async(std::launch::async, &save_to_file, "data4.txt");

    auto f1 = std::async(std::launch::async, &save_to_file, "data1.txt");
    auto f2 = std::async(std::launch::async, &save_to_file, "data2.txt");
    auto f3 = std::async(std::launch::async, &save_to_file, "data3.txt");
    auto f4 = std::async(std::launch::async, &save_to_file, "data4.txt");
}

void shared_future()
{
    std::future<int> f_sqaure_665 = std::async(std::launch::async, &calculate_square, 665);

    std::shared_future<int> sf = f_sqaure_665.share();

    {
        std::jthread thd_1{[sf] { 
            std::cout << "Start#" << std::this_thread::get_id() << std::endl;
            std::cout << "result: " << sf.get() << std::endl;
        }};

        std::jthread thd_2{[sf] { 
            std::cout << "Start#" << std::this_thread::get_id() << std::endl;
            std::cout << "result: " << sf.get() << std::endl;
        }};
    }
}

template <typename TTask>
auto spawn_task(TTask&& task)
{
    using TResult = decltype(task());
    std::packaged_task<TResult()> pt(std::forward<TTask>(task));
    auto f_result = pt.get_future();
    
    std::jthread thd{std::move(pt)}; // async call of task
    thd.detach();

    return f_result;
}

void packaged_tasks()
{
    std::packaged_task<int(int)> pt_square(&calculate_square);
    std::future<int> f_square = pt_square.get_future();

    std::jthread thd{std::move(pt_square), 13}; // asynchronous call
    //pt_square(13); // synchronous call

    std::cout << "13 * 13 = " << f_square.get() << std::endl;

    std::cout << "--------" << std::endl;

    spawn_task([] { save_to_file("data1.txt"); });
    spawn_task([] { save_to_file("data2.txt"); });
    spawn_task([] { save_to_file("data3.txt"); });
    spawn_task([] { save_to_file("data4.txt"); });

    std::vector<std::tuple<int, std::future<int>>> f_squares;

    for (int i = 1; i <= 20; ++i)
    {
        f_squares.emplace_back(i, spawn_task([i] { return calculate_square(i); }));
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

class SquareCalculator
{
    std::promise<int> promise_result_;
public:
    std::future<int> get_future()
    {
        return promise_result_.get_future();
    }

    void calculate(int n)
    {
        try
        {
            int result = calculate_square(n);
            promise_result_.set_value(result);
        }
        catch(const std::exception& e)
        {
            promise_result_.set_exception(std::current_exception());
        }        
    }
};

void promise_demo()
{
    SquareCalculator calc;
    auto f = calc.get_future();

    std::jthread thd{[&calc] { calc.calculate(13); }};

    std::jthread thd_consumer{[f = std::move(f)]() mutable {
        std::cout << "Result: " << f.get() << std::endl;
    }};
}

int main()
{
    //future_grouped_tasks();

    //bug_in_standard();

    //shared_future();

    //packaged_tasks();

    promise_demo();
}
