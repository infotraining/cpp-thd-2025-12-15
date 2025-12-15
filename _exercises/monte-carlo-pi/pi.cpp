#include <atomic>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <numeric>
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
            ++hits; // hot-loop
    }
}

void calculate_hits_with_local_counter(uintmax_t N, uintmax_t& hits)
{
    const size_t seed = std::hash<std::thread::id>{}(std::this_thread::get_id());
    std::mt19937_64 rnd_gen{seed};
    std::uniform_real_distribution<double> rnd_distr{0.0, 1.0};

    uintmax_t local_hits{};
    for (uintmax_t n = 0; n < N; ++n)
    {
        double x = rnd_distr(rnd_gen);
        double y = rnd_distr(rnd_gen);
        if (x * x + y * y <= 1)
            ++local_hits; // hot-loop
    }

    hits += local_hits;
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
    const auto threads_count{std::max(std::thread::hardware_concurrency(), 1u)};

    std::vector<std::thread> threads(threads_count);
    std::vector<uintmax_t> hits(threads_count);
    const uintmax_t n_per_thread = N / threads_count;

    std::cout << "Pi (multi thread)! Number of threads: " << threads_count << std::endl;
    const auto start = chrono::high_resolution_clock::now();

    for (uint32_t i{0}; i < threads_count; i++)
    {
        threads[i] = std::thread(&calculate_hits, static_cast<uintmax_t>(n_per_thread), std::ref(hits[i]));
    }

    for (uint32_t i{0}; i < threads_count; i++)
    {
        threads[i].join();
    }

    uintmax_t total_hits = std::accumulate(hits.begin(), hits.end(), uintmax_t{});
    const double pi = static_cast<double>(total_hits) / N * 4;

    const auto end = chrono::high_resolution_clock::now();
    const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    cout << "Pi = " << pi << endl;
    cout << "Elapsed (" << threads_count << " threads) = " << elapsed_time << "ms" << endl;
}

void multi_thread_pi_with_local_counter()
{
    const auto threads_count{std::max(std::thread::hardware_concurrency(), 1u)};

    std::vector<std::thread> threads(threads_count);
    std::vector<uintmax_t> hits(threads_count);
    const uintmax_t n_per_thread = N / threads_count;

    std::cout << "Pi (multi thread - local counter)! Number of threads: " << threads_count << std::endl;
    const auto start = chrono::high_resolution_clock::now();

    for (uint32_t i{0}; i < threads_count; i++)
    {
        threads[i] = std::thread(&calculate_hits_with_local_counter, static_cast<uintmax_t>(n_per_thread), std::ref(hits[i]));
    }

    for (uint32_t i{0}; i < threads_count; i++)
    {
        threads[i].join();
    }

    uintmax_t total_hits = std::accumulate(hits.begin(), hits.end(), uintmax_t{});
    const double pi = static_cast<double>(total_hits) / N * 4;

    const auto end = chrono::high_resolution_clock::now();
    const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    cout << "Pi = " << pi << endl;
    cout << "Elapsed (" << threads_count << " threads - local counter) = " << elapsed_time << "ms" << endl;
}

struct Hits
{
    alignas(std::hardware_destructive_interference_size) uintmax_t value;
};

void calculate_hits_with_padding(uintmax_t N, Hits& hits)
{
    const size_t seed = std::hash<std::thread::id>{}(std::this_thread::get_id());
    std::mt19937_64 rnd_gen{seed};
    std::uniform_real_distribution<double> rnd_distr{0.0, 1.0};

    for (uintmax_t n = 0; n < N; ++n)
    {
        double x = rnd_distr(rnd_gen);
        double y = rnd_distr(rnd_gen);
        if (x * x + y * y <= 1)
            ++(hits.value); // hot-loop
    }
}

void multi_thread_pi_with_padding()
{
    const auto threads_count{std::max(std::thread::hardware_concurrency(), 1u)};

    std::vector<std::thread> threads(threads_count);
    std::vector<Hits> hits(threads_count);
    const uintmax_t n_per_thread = N / threads_count;

    std::cout << "Pi (multi thread - padding)! Number of threads: " << threads_count << std::endl;
    const auto start = chrono::high_resolution_clock::now();

    for (uint32_t i{0}; i < threads_count; i++)
    {
        threads[i] = std::thread(&calculate_hits_with_padding, static_cast<uintmax_t>(n_per_thread), std::ref(hits[i]));
    }

    for (uint32_t i{0}; i < threads_count; i++)
    {
        threads[i].join();
    }

    uintmax_t total_hits = std::accumulate(hits.begin(), hits.end(), uintmax_t{}, [](uintmax_t red, Hits arg) { return red + arg.value; });
    const double pi = static_cast<double>(total_hits) / N * 4;

    const auto end = chrono::high_resolution_clock::now();
    const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    cout << "Pi = " << pi << endl;
    cout << "Elapsed (" << threads_count << " threads - padding) = " << elapsed_time << "ms" << endl;
}

int main()
{
    const uintmax_t N = 100'000'000;

    single_thread_pi();

    std::cout << "\n----------\n"
              << std::endl;

    multi_thread_pi();

    std::cout << "\n----------\n"
              << std::endl;

    multi_thread_pi_with_local_counter();

    std::cout << "\n----------\n"
              << std::endl;

    multi_thread_pi_with_padding();
}
