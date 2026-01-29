#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include "trace_span.h"

class MutexQueue
{
public:
    void push(const TraceSpan &span)
    {
        std::unique_lock<std::mutex> lock(mtx_);
        q_.push(span);
        lock.unlock();
        not_empty_.notify_one();
    }

    bool pop(TraceSpan &span)
    {
        std::unique_lock<std::mutex> lock(mtx_);
        if (q_.empty())
        {
            return false;
        }
        span = q_.front();
        q_.pop();
        return true;
    }

private:
    std::queue<TraceSpan> q_;
    std::mutex mtx_;
    std::condition_variable not_empty_;
};