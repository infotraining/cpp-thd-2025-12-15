#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <variant>
#include <vector>

using namespace std::literals;

template <typename... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};

// deduction guide - C++17 - implicitly provided since C++20
// template <typename... Ts>
// overloaded(Ts...) -> overloaded<Ts...>;

template <typename T>
class Result
{
    // T value_;
    // std::exception_ptr eptr_;
    std::variant<std::monostate, T, std::exception_ptr> value_;

public:
    void set_exception(std::exception_ptr eptr)
    {
        value_ = eptr;
    }

    template <typename TValue>
    void set_value(TValue&& value)
    {
        value_ = std::forward<TValue>(value);
    }

    T get()
    {
        // struct ValueVisitor
        // {
        //     T operator()(const T& v) const
        //     {
        //         return v;
        //     }

        //     T operator()(std::exception_ptr e) const
        //     {
        //         std::rethrow_exception(e);
        //     }

        //     T operator()(std::monostate) const
        //     {
        //         throw std::logic_error("Result/Exception is not set");
        //     }
        // };

        // ValueVisitor value_visitor;
        // return std::visit(value_visitor, value_);

        auto value_visitor = overloaded{
            [](const T& v) -> T {
                return v;
            },
            [](std::exception_ptr e) -> T {
                std::rethrow_exception(e);
            },
            [](std::monostate) -> T {
                throw std::logic_error("Result/Exception is not set");
            }};

        return std::visit(value_visitor, value_);
    }
};

void background_work(size_t id, const std::string& text, Result<char>& result)
{
    try
    {
        std::cout << "bw#" << id << " has started..." << std::endl;

        for (const auto& c : text)
        {
            std::cout << "bw#" << id << ": " << c << std::endl;

            std::this_thread::sleep_for(100ms);
        }

        result.set_value(text.at(5)); // potential exception

        std::cout << "bw#" << id << " is finished..." << std::endl;
    }
    catch (...)
    {
        result.set_exception(std::current_exception());
        return;
    }
}

int main()
{
    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello_Threads";

    const size_t no_of_threads = 2;

    std::vector<Result<char>> results(no_of_threads);

    {
        std::jthread thd_1{&background_work, 1, text, std::ref(results[0])};
        std::jthread thd_2{&background_work, 2, "bad", std::ref(results[1])};
    } // implicit join

    for (size_t i = 0; i < no_of_threads; ++i)
    {
        try
        {
            char result = results[i].get();
            std::cout << "result_" << std::to_string(i) << ": " << result << std::endl;
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

    std::cout << "Main thread ends..." << std::endl;
}
