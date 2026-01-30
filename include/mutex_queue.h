#pragma once

#include <queue>
#include <mutex>
#include <vector>

template <typename T>
class MutexQueue
{
public:
    explicit MutexQueue(size_t capacity) : capacity_(capacity) {}

    bool enqueue(const T &data)
    {
        std::unique_lock<std::mutex> lock(mtx_);

        if (q_.size() >= capacity_)
        {
            return false;
        }

        q_.push(data);
        return true;
    }

    bool dequeue(T &data)
    {
        std::unique_lock<std::mutex> lock(mtx_);

        if (q_.empty())
        {
            return false;
        }

        data = q_.front();
        q_.pop();
        return true;
    }

private:
    std::queue<T> q_;
    std::mutex mtx_;
    size_t capacity_;
};