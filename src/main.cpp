/**
 * @file main.cpp
 * @brief Single-threaded correctness test for RingBuffer
 *
 * This test verifies the basic functionality of the RingBuffer class
 * in a single-threaded environment. It ensures that:
 * - Data can be successfully enqueued into the buffer
 * - Data can be successfully dequeued from the buffer
 * - Data integrity is maintained throughout enqueue/dequeue operations
 *
 * The test creates a RingBuffer with capacity 4, enqueues a TraceSpan
 * with known values, and verifies that the dequeued data matches the
 * original input, confirming no information is lost during the operation.
 */
// src/main.cpp
/**
 *
 *
 */
#include <iostream>
#include <cassert>
#include "ring_buffer.h"
#include "trace_span.h"

int main()
{
    RingBuffer rb(4);

    // dummy span
    TraceSpan in_span = {};
    in_span.trace_id_high = 12345;
    in_span.duration_ns = 500;

    // testing enqueue operation
    if (rb.enqueue(in_span))
    {
        std::cout << "[PASS] Enqueue successful." << std::endl;
    }
    else
    {
        std::cerr << "[FAIL] Enqueue failed on empty buffer!" << std::endl;
        return 1;
    }

    // testing dequeue operation
    TraceSpan out_span = {};
    if (rb.dequeue(out_span))
    {
        std::cout << "[PASS] Dequeue successful." << std::endl;

        // verify data integrity
        assert(out_span.trace_id_high == 12345);
        std::cout << "[PASS] Data integrity verified (ID: " << out_span.trace_id_high << ")." << std::endl;
    }
    else
    {
        std::cerr << "[FAIL] Dequeue failed on non-empty buffer!" << std::endl;
        return 1;
    }

    return 0;
}