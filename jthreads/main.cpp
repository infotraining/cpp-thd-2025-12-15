#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals;

void background_work(std::stop_token stop_tkn, size_t id, const std::string& text, std::chrono::milliseconds delay)
{
    std::cout << "bw#" << id << " has started..." << " - thread_id#" << std::this_thread::get_id() << std::endl;

    for (const auto& c : text)
    {
        if (stop_tkn.stop_requested())
        {
            std::cout << "Stop has been requested for bw#" << id << " - thread_id#" << std::this_thread::get_id() <<  "..." << std::endl;
            return;
        }
        std::cout << "bw#" << id << ": " << c << std::endl;

        std::this_thread::sleep_for(delay);
    }

    std::cout << "bw#" << id << " - thread_id#" << std::this_thread::get_id() << " is finished..." << std::endl;
}

class BackgroundWork
{
    const int id_;
    const std::string text_;

public:
    BackgroundWork(int id, std::string text)
        : id_{id}
        , text_{std::move(text)}
    {
    }

    void operator()(std::chrono::milliseconds delay) const
    {
        std::cout << "BW#" << id_ << " has started..." << std::endl;

        for (const auto& c : text_)
        {
            std::cout << "BW#" << id_ << ": " << c << std::endl;

            std::this_thread::sleep_for(delay);
        }

        std::cout << "BW#" << id_ << " is finished..." << std::endl;
    }
};

int main()
{
    const size_t no_of_cores = std::max(std::thread::hardware_concurrency(), 1u);
    std::cout << "No of cores: " << no_of_cores << std::endl;
 
    std::cout << "Main thread starts..." << std::endl;
    
    std::stop_source stop_src;

    {
        auto text = std::make_shared<const std::string>("Hello Threads");
        
        const std::string& ref_text = *text;
        std::jthread thd_1{&background_work, 1, std::cref(ref_text), 600ms }; // internal stop token used thd_1

        auto delay = 800ms;
        auto cancel_token = stop_src.get_token();
        std::jthread thd_2{ [=] { background_work(cancel_token, 2, *text, delay); }};

        std::jthread thd_cancel{[&stop_src] { 
            std::this_thread::sleep_for(2s);
            std::cout << "Cancel all tasks..." << std::endl;
            stop_src.request_stop();
        }};
    }

    std::cout << "Main thread ends..." << std::endl;
} 
