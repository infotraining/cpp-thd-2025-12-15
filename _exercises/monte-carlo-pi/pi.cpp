#include <atomic>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <random>
#include <thread>

using namespace std;

const uintmax_t N = 500'000'000;

void calculate_hits(uintmax_t N, uintmax_t& hits)
{
    for (uintmax_t n = 0; n < N; ++n)
    {
        double x = rand() / static_cast<double>(RAND_MAX);
        double y = rand() / static_cast<double>(RAND_MAX);
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

int main()
{
    const uintmax_t N = 100'000'000;

    single_thread_pi();
}
