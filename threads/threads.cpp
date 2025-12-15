#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals;

void background_work(size_t id, const std::string& text, std::chrono::milliseconds delay)
{
    std::cout << "bw#" << id << " has started..." << std::endl;

    for (const auto& c : text)
    {
        std::cout << "bw#" << id << ": " << c << std::endl;
  
        std::this_thread::sleep_for(delay);
    }

    std::cout << "Text: " << text << "\n";
    std::cout << "bw#" << id << " is finished..." << std::endl;
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

std::thread create_thread(std::chrono::milliseconds delay) 
{
    static int _id = 0;
    const int id = ++_id;
    std::string text = "BackgroundWork#" + std::to_string(id);

    return std::thread{&background_work, id, text, delay};
}

int main()
{
    std::cout << "Main thread starts..." << std::endl;
    std::string text = "Hello Threads";

    std::thread thd; // thd empty - no os resources allocated
    std::cout << "Empty thread id: " << thd.get_id() << "\n";
    thd = std::thread{&background_work, 3, std::cref(text), 100ms}; // no empty any more - os thread assigned

    std::thread thd_1{&background_work, 1, "Hello", 200ms};
    std::thread thd_2{&background_work, 2, "Threads...", 350ms};

    BackgroundWork bw_4{4, "Multi-threading"};
    std::thread thd_4(std::ref(bw_4), 50ms);

    auto delay = 500ms;
    std::thread thd_5([delay, text = std::move(text)] { background_work(5, text, delay); });
    std::thread thd_6 = create_thread(400ms);
    std::thread thd_7 = create_thread(500ms);

    thd_1.join();
    thd_2.join();
    thd.join();
    thd_4.join();
    thd_5.detach();
    thd_6.join();
    thd_7.join();

    assert(not thd_5.joinable());

    std::cout << "Main thread ends..." << std::endl;
}
