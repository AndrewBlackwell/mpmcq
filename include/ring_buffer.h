/**
 * A Lock-Free Multi-Producer Multi-Consumer (MPMC) Ring Buffer loosely based on
 * Dmitry Vyukov's MPMC bounded queue algorithm.
 *
 * This is a thread-safe ring buffer that allows multiple producers
 * to enqueue elements and multiple consumers to dequeue elements concurrently
 * without using locks. The buffer uses atomic operations and a turn-based
 * coordination mechanism to synchronize access.
 */
#pragma once

#include <vector>
#include <atomic>
#include <cstddef>
#include <cassert>
#include "trace_span.h"

template <typename T>
class RingBuffer
{
public:
    // in order to use fast bitwise masking, require capacity to be a power of two
    // initializing buffer_ in the member list allows is to hold non-movable atomic types
    explicit RingBuffer(size_t capacity)
        : capacity_(capacity),
          mask_(capacity - 1),
          buffer_(capacity)
    {
        // if (x & (x-1) == 0), x is a power of two.
        assert((capacity > 0) && ((capacity & (capacity - 1)) == 0));

        // initialize the "turn" flags, slot x gets x.
        // this coordinates the first "lap" through the ring
        for (size_t i = 0; i < capacity_; ++i)
        {
            buffer_[i].flags_.store(i, std::memory_order_relaxed);
        }

        // initialize the head and tail
        head_.data.store(0, std::memory_order_relaxed);
        tail_.data.store(0, std::memory_order_relaxed);
    }

    // Producer tries to enqueue a TraceSpan into the ring.
    // returns true on success, false if the ring is full.
    bool enqueue(const T &data)
    {
        // load the head postion
        size_t head = head_.data.load(std::memory_order_relaxed);
        for (;;)
        {
            // equivalent to head % capacity;
            size_t slot_idx = head & mask_;

            Node &node = buffer_[slot_idx];

            // reading the turn counter for this slot
            size_t turn = node.flags_.load(std::memory_order_acquire);
            // is it our turn to write here?
            // if turn == head, then yes!
            int64_t diff = (int64_t)turn - (int64_t)head;

            if (diff == 0)
            {
                // let's attempt to reserve this slot by moving head forward
                if (head_.data.compare_exchange_weak(head, head + 1, std::memory_order_relaxed))
                {
                    // success! we now own this slot. let's write our data.
                    // this work is non-atomic, parallel!
                    node.data_ = data;

                    node.flags_.store(head + 1, std::memory_order_release);
                    return true;
                }
                // if the CAS failed, head is updated to the new value.
            }
            else if (diff < 0)
            {
                // this means the buffer is full.
                // For example, we want to write to #100, but the flag is at #90.
                return false;
            }
            else
            {
                // edge-case: someone else beat us to this slot and incremented head.
                // let's reload head and try again.
                head = head_.data.load(std::memory_order_relaxed);
            }
        }
    }

    // Consumer tries to dequeue a TraceSpan from the ring.
    // returns true on success, false if the ring is empty.
    bool dequeue(T &data)
    {
        size_t tail = tail_.data.load(std::memory_order_relaxed);

        for (;;)
        {
            size_t slot_idx = tail & mask_;

            Node &node = buffer_[slot_idx];

            size_t turn = node.flags_.load(std::memory_order_acquire);
            // if turn == (tail + 1), then the writer has
            // finished writing here and we are ready to read!
            int64_t diff = (int64_t)turn - (int64_t)(tail + 1);

            if (diff == 0)
            {
                // let's attempt to reserve this read
                if (tail_.data.compare_exchange_weak(tail, tail + 1, std::memory_order_relaxed))
                {
                    // success! copy data out:
                    data = node.data_;

                    // now we must publish the read, which opens
                    // the slot for the next writer. The next writer
                    // will be looking for this current tail + capacity,
                    // since it will be the next "lap" around the ring
                    node.flags_.store(tail + capacity_, std::memory_order_release);
                    return true;
                }
            }
            else if (diff < 0)
            {
                // empty, the writer hasn't finished here yet
                return false;
            }
            else
            {
                // someone else beat us here
                tail = tail_.data.load(std::memory_order_relaxed);
            }
        }
    }

private:
    // bounding flags + data prevents false sharing
    struct Node
    {
        std::atomic<size_t> flags_;
        T data_;
    };

    size_t capacity_;
    size_t mask_;
    std::vector<Node> buffer_;

    struct alignas(128) AlignedAtomic
    {
        std::atomic<size_t> data;
    };

    AlignedAtomic head_;
    AlignedAtomic tail_;
};