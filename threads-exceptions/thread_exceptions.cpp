#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals;

void background_work(size_t id, const std::string& text, char& result, std::exception_ptr& eptr)
{
    try
    {
        std::cout << "bw#" << id << " has started..." << std::endl;

        for (const auto& c : text)
        {
            std::cout << "bw#" << id << ": " << c << std::endl;

            std::this_thread::sleep_for(100ms);
        }

        result = text.at(5); // potential exception

        std::cout << "bw#" << id << " is finished..." << std::endl;
    }
    catch (...)
    {
        eptr = std::current_exception();
        return;
    }
}

int main()
{
    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello_Threads";

    const size_t no_of_threads = 2;

    std::vector<char> results(2);
    std::vector<std::exception_ptr> exceptions(2);

    {
        std::jthread thd_1{&background_work, 1, text, std::ref(results[0]), std::ref(exceptions[0])};
        std::jthread thd_2{&background_work, 2, "bad", std::ref(results[1]), std::ref(exceptions[1])};
    } // implicit join

    for (size_t i = 0; i < no_of_threads; ++i)
    {
        if (exceptions[i])
        {
            try
            {
                std::rethrow_exception(exceptions[i]);
            }
            catch (const std::out_of_range& e)
            {
                std::cout << "Out of range caught!!! ";
                std::cout << "Error caught: " << e.what() << std::endl;
            }
            catch (const std::exception& e)
            {
                std::cout << "Error caught: " << e.what() << std::endl;
            }
        }
        else
        {
            std::cout << "result_" << std::to_string(i) << results[0] << std::endl;
        }
    }

    std::cout << "Main thread ends..." << std::endl;
}
