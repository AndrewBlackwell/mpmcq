#pragma once

#include <vector>
#include <atomic>
#include <cstddef>
#include <cassert>
#include "trace_span.h"

/** A Lock-Free Multi-Producer Multi-Consumer (MPMC) Ring Buffer based on
 * Dmitry Vyukov's MPMC bounded queue algorithm.
 **/
class RingBuffer
{
public:
    // in order to use fast bitwise masking, require capacity to be a power of two
    explicit RingBuffer(size_t capacity) : capacity_(capacity), mask_(capacity - 1)
    {
        // if (x & (x-1) == 0), x is a power of two.
        assert((capacity > 0) && ((capacity & (capacity - 1)) == 0));

        // pre-allocate the data buffer
        buffer_.resize(capacity_);

        // initialize the "turn" flags, slot x gets x.
        // this coordinates the first "lap" through the ring
        flags_.resize(capacity_);
        for (size_t i = 0; i < capacity_; ++i)
        {
            flags_[i].store(i, std::memory_order_relaxed);
        }

        // initialize the head and tail
        head_.data.store(0, std::memory_order_relaxed);
        tail_.data.store(0, std::memory_order_relaxed);
    }

private:
    std::vector<TraceSpan> buffer_;
    size_t capacity_;
    size_t mask_;

    /**
     * The Sequence / Turn Array
     * std::atomic is not copyable, so we can't just resize() a vector of them
     * easily without a custom allocator, but for simplicity here std::vector
     * is okay if initialized in constructor size.
     **/
    std::vector<std::atomic<size_t>> flags_;

    /**
     * To prevent false sharing, we put head and tail on their own cache lines.
     * Since I'm on Apple Silicon, I'm using 128 bytes.
     **/
    struct alignas(128) AlignedAtomic
    {
        std::atomic<size_t> data;
    };

    AlignedAtomic head_; // producers write here
    AlignedAtomic tail_; // consumers write here
};