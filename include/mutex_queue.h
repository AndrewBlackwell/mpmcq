#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include "trace_span.h"

class MutexQueue
{
public:
    explicit MutexQueue(size_t capacity) : capacity_(capacity) {}

    void enqueue(const TraceSpan &span)
    {
        std::unique_lock<std::mutex> lock(mtx_);

        not_full_.wait(lock, [this]()
                       { return q_.size() < capacity_; });

        q_.push(span);

        // unlock before notifying to avoid
        // waking up to a locked mutex
        lock.unlock();
        not_empty_.notify_one();
    }

    bool dequeue(TraceSpan &span)
    {
        std::unique_lock<std::mutex> lock(mtx_);

        not_empty_.wait(lock, [this]()
                        { return !q_.empty(); });

        span = q_.front();
        q_.pop();
        lock.unlock();

        // wake up a sleeping producer
        not_full_.notify_one();

        return true;
    }

private:
    std::queue<TraceSpan> q_;
    std::mutex mtx_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    size_t capacity_;
};