#include <atomic>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <random>
#include <thread>

using namespace std;

const uintmax_t N = 100'000'000;

void calculate_hits(uintmax_t N, uintmax_t& hits)
{
    const size_t seed = std::hash<std::thread::id>{}(std::this_thread::get_id());
    std::mt19937_64 rnd_gen{seed};
    std::uniform_real_distribution<double> rnd_distr{0.0, 1.0};

    for (uintmax_t n = 0; n < N; ++n)
    {
        double x = rnd_distr(rnd_gen);
        double y = rnd_distr(rnd_gen);
        if (x * x + y * y <= 1)
            ++hits;
    }
}

void single_thread_pi()
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    cout << "Pi (single thread)!" << endl;
    const auto start = chrono::high_resolution_clock::now();

    uintmax_t hits = 0;

    calculate_hits(N, hits);

    const double pi = static_cast<double>(hits) / N * 4;

    const auto end = chrono::high_resolution_clock::now();
    const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    cout << "Pi = " << pi << endl;
    cout << "Elapsed (single thread) = " << elapsed_time << "ms" << endl;
}

void multi_thread_pi()
{
    const auto noThreads{std::max(std::thread::hardware_concurrency(), 1u)};

    std::vector<std::thread> threads(noThreads);
    std::vector<uintmax_t> noHits(noThreads);
    const uintmax_t n_per_thread = N / noThreads;

    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    std::cout << "Pi (multi thread)! Number of threads: " << noThreads << std::endl;
    const auto start = chrono::high_resolution_clock::now();

    for (uint32_t i{0}; i < noThreads; i++)
    {
        threads[i] = std::thread(&calculate_hits, static_cast<uintmax_t>(n_per_thread), std::ref(noHits[i]));
    }

    for (uint32_t i{0}; i < noThreads; i++)
    {
        threads[i].join();
    }

    uint32_t allHits{};
    for (uint32_t i{0}; i < noThreads; i++)
    {
        allHits += noHits[i];
    }

    const double pi = static_cast<double>(allHits) / N * 4;

    const auto end = chrono::high_resolution_clock::now();
    const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    cout << "Pi = " << pi << endl;
    cout << "Elapsed (" << noThreads << " threads) = " << elapsed_time << "ms" << endl;
}

int main()
{
    const uintmax_t N = 100'000'000;

    single_thread_pi();

    multi_thread_pi();
}
