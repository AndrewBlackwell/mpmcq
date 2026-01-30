#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <iomanip>
#include <string>
#include <sstream>

#include "ring_buffer.h"
#include "mutex_queue.h"
#include "cpu_relax.h"  // hardware pause instruction
#include "trace_span.h" // an example 128-byte payload simulating a trace-span

struct BenchmarkConfig
{
    std::string name;
    int num_producers;
    int num_consumers;
    size_t queue_capacity;
    int duration_seconds;
};

struct BenchmarkResult
{
    long long total_ops;
    double ops_per_sec;
};

// Helper to format large numbers (e.g., 14,242,841)
std::string format_num(long long n)
{
    std::stringstream ss;
    ss.imbue(std::locale(""));
    ss << std::fixed << n;
    return ss.str();
}

template <typename QueueType, typename PayloadType>
BenchmarkResult run_benchmark(const BenchmarkConfig &config)
{
    QueueType queue(config.queue_capacity);

    std::atomic<long long> enqueue_count(0);
    std::atomic<long long> dequeue_count(0);
    std::atomic<bool> stop_flag(false);
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    // launch producer threads
    for (int i = 0; i < config.num_producers; ++i)
    {
        producers.emplace_back([&]()
                               {
            PayloadType item = {};
            while (!stop_flag.load(std::memory_order_relaxed)) {
                if (queue.enqueue(item)) {
                    enqueue_count.fetch_add(1, std::memory_order_relaxed);
                } else {
                    cpu_relax();
                }
            } });
    }

    // launch consumer threads
    for (int i = 0; i < config.num_consumers; ++i)
    {
        consumers.emplace_back([&]()
                               {
            PayloadType item;
            while (!stop_flag.load(std::memory_order_relaxed)) {
                if (queue.dequeue(item)) {
                    dequeue_count.fetch_add(1, std::memory_order_relaxed);
                } else {
                    cpu_relax();
                }
            } });
    }

    // run for the specified duration
    std::this_thread::sleep_for(std::chrono::seconds(config.duration_seconds));
    stop_flag.store(true, std::memory_order_relaxed);

    // join threads
    for (auto &t : producers)
        t.join();
    for (auto &t : consumers)
        t.join();

    long long total = enqueue_count + dequeue_count;
    return {total, (double)total / config.duration_seconds};
}

void print_header()
{
    std::cout << std::endl
              << std::left << std::setw(25) << "Scenario"
              << std::setw(15) << "Threads"
              << std::setw(20) << "Mutex (Ops/s)"
              << std::setw(20) << "Lock-Free (Ops/s)"
              << std::setw(10) << "Speedup"
              << std::endl
              << std::string(90, '-') << std::endl;
}

void run_comparison(const std::string &name, int prod, int cons, size_t cap, int duration)
{
    BenchmarkConfig config{name, prod, cons, cap, duration};

    // mutex-queue (baseline)
    auto res_mutex = run_benchmark<MutexQueue<TraceSpan>, TraceSpan>(config);

    // lock-free ring buffer implementation
    auto res_ring = run_benchmark<RingBuffer<TraceSpan>, TraceSpan>(config);

    double speedup = res_ring.ops_per_sec / res_mutex.ops_per_sec;
    std::string thread_str = std::to_string(prod) + "P / " + std::to_string(cons) + "C";

    std::cout << std::left << std::setw(25) << name
              << std::setw(15) << thread_str
              << std::setw(20) << format_num((long long)res_mutex.ops_per_sec)
              << std::setw(20) << format_num((long long)res_ring.ops_per_sec)
              << std::fixed << std::setprecision(2) << speedup << "x"
              << std::endl;
}

int main()
{
    std::cout << "==========================================================================================" << std::endl;
    std::cout << "  Payload: 128 bytes (TraceSpan) | Default Capacity: 65536 | Duration: 4s per test" << std::endl;
    std::cout << "==========================================================================================" << std::endl;

    print_header();

    // baseline latency test: low contention
    // one producer thread, one consumer thread
    run_comparison("baseline, 1p1c", 1, 1, 65536, 4);

    // balanced load test: moderate contention
    // two producer threads, two consumer threads
    run_comparison("balanced, 2p2c", 2, 2, 65536, 4);

    // crowded contention test: high contention
    // four producer threads, four consumer threads
    run_comparison("crowded, 4p4c", 4, 4, 65536, 4);

    // oversubscription test: saturated contention
    // eight producer threads, eight consumer threads
    // forces os context switches on most systems
    run_comparison("oversubscribed, 8p8c", 8, 8, 65536, 4);

    // small capacity, moderate contention
    std::cout << std::string(90, '-') << std::endl;
    std::cout << "stress test: small capacity (1024)" << std::endl;
    run_comparison("small capacity, 2p2c", 2, 2, 1024, 4);

    std::cout << std::endl
              << "Done" << std::endl;
    return 0;
}