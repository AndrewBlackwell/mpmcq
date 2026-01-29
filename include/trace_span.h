#pragma once
#include <cstdint>
#include <new> //hardware_desrtuctive_intererference_size

constexpr std::size_t CACHE_LINE_SIZE = 64;

struct alignas(CACHE_LINE_SIZE) TraceSpan
{
    uint64_t trace_id_high;
    uint64_t trace_id_low;
    uint64_t span_id;
    uint64_t parent_id;
    uint64_t start_ns;
    uint64_t duration_ns;
    uint32_t flags;
    // alignas to ensure that no two Spans share a cache line
};
