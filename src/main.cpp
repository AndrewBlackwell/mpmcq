// src/main.cpp
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

    // testing push operation
    if (rb.push(in_span))
    {
        std::cout << "[PASS] Push successful." << std::endl;
    }
    else
    {
        std::cerr << "[FAIL] Push failed on empty buffer!" << std::endl;
        return 1;
    }

    // testing pop operation
    TraceSpan out_span = {};
    if (rb.pop(out_span))
    {
        std::cout << "[PASS] Pop successful." << std::endl;

        // verify data integrity
        assert(out_span.trace_id_high == 12345);
        std::cout << "[PASS] Data integrity verified (ID: " << out_span.trace_id_high << ")." << std::endl;
    }
    else
    {
        std::cerr << "[FAIL] Pop failed on non-empty buffer!" << std::endl;
        return 1;
    }

    return 0;
}