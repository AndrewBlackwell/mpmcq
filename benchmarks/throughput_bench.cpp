#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <iomanip>

#include "mutex_queue.h"
#include "ring_buffer.h"
#include "trace_span.h"

// test constants
const size_t QUEUE_CAPACITY = 1024;
const size_t TOTAL_OPS = 10'000'000;
const int THREAD_COUNT = 4;

void benchMutexQueue()
{
    MutexQueue queue;
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    size_t ops_per_thread = TOTAL_OPS / THREAD_COUNT;

    auto start = std::chrono::high_resolution_clock::now();

    // launch consumer threads
    for (int i = 0; i < THREAD_COUNT; ++i)
    {
        consumers.emplace_back([&queue, ops_per_thread]()
                               {
            TraceSpan span;
            for (size_t j = 0; j < ops_per_thread; ++j)
            {
                while (!queue.pop(span))
                {
                    std::this_thread::yield();
                }
            } });
    }

    // launch producer threads
    for (int i = 0; i < THREAD_COUNT; ++i)
    {
        producers.emplace_back([&queue, ops_per_thread]()
                               {
            TraceSpan span{};
            for (size_t j = 0; j < ops_per_thread; ++j)
            {
                queue.push(span);
            } });
    }

    // join all
    for (auto &t : producers)
        t.join();
    for (auto &t : consumers)
        t.join();

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> duration = end - start;
    double throughput = TOTAL_OPS / duration.count();

    std::cout << "[MutexQueue] Time: " << std::setprecision(5) << duration.count() << " seconds | Throughput: " << (long)(throughput) << " ops/sec" << std::endl;
}

void benchRingBuffer()
{
    RingBuffer queue(QUEUE_CAPACITY);
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    size_t ops_per_thread = TOTAL_OPS / THREAD_COUNT;

    auto start = std::chrono::high_resolution_clock::now();

    // launch consumer threads
    for (int i = 0; i < THREAD_COUNT; ++i)
    {
        consumers.emplace_back([&queue, ops_per_thread]()
                               {
            TraceSpan span;
            for (size_t j = 0; j < ops_per_thread; ++j)
            {
                // if empty, keep trying
                while (!queue.pop(span))
                {
                    // optional, I'm being nice to my CPU here
                    // std::this_thread::yield();
                }
            } });
    }

    // launch producer threads
    for (int i = 0; i < THREAD_COUNT; ++i)
    {
        producers.emplace_back([&queue, ops_per_thread]()
                               {
            TraceSpan span{};
            for (size_t j = 0; j < ops_per_thread; ++j)
            {
                // if full, keep trying
                while (!queue.push(span))
                {
                    // std::this_thread::yield();
                }
            } });
    }

    // join all
    for (auto &t : producers)
        t.join();
    for (auto &t : consumers)
        t.join();

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> duration = end - start;
    double throughput = TOTAL_OPS / duration.count();

    std::cout << "[RingBuffer] Time: " << std::setprecision(5) << duration.count() << " seconds | Throughput: " << (long)(throughput) << " ops/sec" << std::endl;
}

int main()
{
    std::cout << "Benchmarking with " << TOTAL_OPS << " items and " << THREAD_COUNT << " threads." << std::endl;

    benchMutexQueue();
    benchRingBuffer();

    return 0;
}