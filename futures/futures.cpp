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

int main()
{
    future_grouped_tasks();
}
